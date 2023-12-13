/**
 * @file
 * @brief Implementation of module manager
 *
 * @copyright Copyright (c) 2017-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "ModuleManager.hpp"
#include "Event.hpp"

#include <dlfcn.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>

#include <TROOT.h>
#include <TSystem.h>

#include "core/config/ConfigManager.hpp"
#include "core/config/Configuration.hpp"
#include "core/config/exceptions.h"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/utils/log.h"

// Common prefix for all modules
// TODO [doc] Should be provided by the build system
#define ALLPIX_MODULE_PREFIX "libAllpixModule"

// These should point to the function defined in dynamic_module_impl.cpp
#define ALLPIX_GENERATOR_FUNCTION "allpix_module_generator"
#define ALLPIX_UNIQUE_FUNCTION "allpix_module_is_unique"

using namespace allpix;

ModuleManager::ModuleManager() : terminate_(false) {}

/**
 * Loads the modules specified in the configuration file. Each module is contained within its own library which is loaded
 * automatically. After that the required modules are created from the configuration.
 */
void ModuleManager::load(Messenger* messenger, ConfigManager* conf_manager, GeometryManager* geo_manager) {
    // Store config manager and get configurations
    conf_manager_ = conf_manager;
    auto& configs = conf_manager_->getModuleConfigurations();
    Configuration& global_config = conf_manager_->getGlobalConfiguration();

    // Set alias for backward compatibility with the previous keyword for multithreading
    global_config.setDefault("multithreading", true);
    multithreading_flag_ = global_config.get<bool>("multithreading");

    // Set default for performance plot creation:
    global_config.setDefault("performance_plots", false);

    // Store the messenger
    messenger_ = messenger;

    // (Re)create the main ROOT file
    auto path = std::filesystem::path(gSystem->pwd()) / global_config.get<std::string>("root_file", "modules");
    path.replace_extension("root");

    if(std::filesystem::is_regular_file(path)) {
        if(global_config.get<bool>("deny_overwrite", false)) {
            throw RuntimeError("Overwriting of existing main ROOT file " + path.string() + " denied");
        }
        LOG(WARNING) << "Main ROOT file " << path << " exists and will be overwritten.";
        std::filesystem::remove(path);
    }
    modules_file_ = std::make_unique<TFile>(path.c_str(), "RECREATE");
    if(modules_file_->IsZombie()) {
        throw RuntimeError("Cannot create main ROOT file " + path.string());
    }
    modules_file_->cd();

    // Loop through all non-global configurations
    for(auto& config : configs) {
        // Load library for each module. Libraries are named (by convention + CMAKE) libAllpixModule Name.suffix
        std::string lib_name = std::string(ALLPIX_MODULE_PREFIX).append(config.getName()).append(SHARED_LIBRARY_SUFFIX);
        LOG_PROGRESS(STATUS, "LOAD_LOOP") << "Loading module " << config.getName();

        void* lib = nullptr;
        bool load_error = false;
        dlerror();
        if(loaded_libraries_.count(lib_name) == 0) {
            // If library is not loaded then try to load it first from the config directories
            if(global_config.has("library_directories")) {
                LOG(TRACE) << "Attempting to load library from configured paths";
                auto lib_paths = global_config.getPathArray("library_directories", true);
                for(const auto& lib_path : lib_paths) {
                    auto full_lib_path = lib_path;
                    full_lib_path /= lib_name;
                    LOG(TRACE) << "Searching in path " << full_lib_path;

                    // Check if the absolute file exists and try to load if it exists
                    std::ifstream check_file(full_lib_path);
                    if(check_file.good()) {
                        lib = dlopen(full_lib_path.c_str(), RTLD_NOW);
                        if(lib != nullptr) {
                            LOG(DEBUG) << "Found library in configuration specified directory at " << full_lib_path;
                        } else {
                            load_error = true;
                        }
                        break;
                    }
                }
            }

            // Otherwise try to load from the standard paths if not found already
            if(!load_error && lib == nullptr) {
                lib = dlopen(lib_name.c_str(), RTLD_NOW);

                if(lib != nullptr) {
                    Dl_info dl_info;
                    dl_info.dli_fname = "";

                    // workaround to get the location of the library
                    int ret = dladdr(dlsym(lib, ALLPIX_UNIQUE_FUNCTION), &dl_info);
                    if(ret != 0) {
                        LOG(DEBUG) << "Found library during global search in runtime paths at " << dl_info.dli_fname;
                    } else {
                        LOG(WARNING)
                            << "Found library during global search but could not deduce location, likely broken library";
                    }
                } else {
                    load_error = true;
                }
            }
        } else {
            // Otherwise just fetch it from the cache
            lib = loaded_libraries_[lib_name];
        }

        // If library did not load then throw exception
        if(load_error) {
            const char* lib_error = dlerror();

            // Find the name of the loaded library if it exists
            std::string lib_error_str = lib_error;
            size_t end_pos = lib_error_str.find(':');
            std::string problem_lib;
            if(end_pos != std::string::npos) {
                problem_lib = lib_error_str.substr(0, end_pos);
            }

            // FIXME is checking the error in this way portable?
            if(lib_error != nullptr && std::strstr(lib_error, "cannot allocate memory in static TLS block") != nullptr) {
                LOG(ERROR) << "Library could not be loaded: not enough thread local storage available" << std::endl
                           << "Try one of below workarounds:" << std::endl
                           << "- Rerun library with the environmental variable LD_PRELOAD='" << problem_lib << "'"
                           << std::endl
                           << "- Recompile the library " << problem_lib << " with tls-model=global-dynamic";
            } else if(lib_error != nullptr && std::strstr(lib_error, "cannot open shared object file") != nullptr &&
                      problem_lib.find(ALLPIX_MODULE_PREFIX) == std::string::npos) {
                LOG(ERROR) << "Library could not be loaded: one of its dependencies is missing" << std::endl
                           << "The name of the missing library is " << problem_lib << std::endl
                           << "Please make sure the library is properly initialized and try again";
            } else if(lib_error != nullptr && std::strstr(lib_error, "undefined symbol") != nullptr) {
                LOG(ERROR) << "Library could not be loaded: library version does not match framework (undefined symbols)"
                           << std::endl
                           << "The name of the problematic library is " << problem_lib << std::endl
                           << "Please make sure the library is compiled against the correct framework version";
            } else {
                LOG(ERROR) << "Library could not be loaded: it is not available" << std::endl
                           << " - Did you enable the library during building? " << std::endl
                           << " - Did you spell the library name correctly (case-sensitive)? ";
                if(lib_error != nullptr) {
                    LOG(DEBUG) << "Detailed error: " << lib_error;
                }
            }

            throw allpix::DynamicLibraryError(config.getName());
        }
        // Remember that this library was loaded
        loaded_libraries_[lib_name] = lib;

        // Check if this module is produced once, or once per detector
        bool unique = true;
        void* uniqueFunction = dlsym(loaded_libraries_[lib_name], ALLPIX_UNIQUE_FUNCTION);

        // If the unique function was not found, throw an error
        if(uniqueFunction == nullptr) {
            LOG(ERROR) << "Module library is invalid or outdated: required interface function not found!";
            throw allpix::DynamicLibraryError(config.getName());
        } else {
            unique = reinterpret_cast<bool (*)()>(uniqueFunction)(); // NOLINT
        }

        // Add the global internal parameters to the configuration
        std::string global_dir = gSystem->pwd();
        config.set<std::string>("_global_dir", global_dir);

        // Set default input and output name
        config.setDefault<std::string>("input", "");
        config.setDefault<std::string>("output", "");

        // Create the modules from the library depending on the module type
        std::vector<std::pair<ModuleIdentifier, Module*>> mod_list;
        if(unique) {
            mod_list.emplace_back(create_unique_modules(loaded_libraries_[lib_name], config, messenger, geo_manager));
        } else {
            mod_list = create_detector_modules(loaded_libraries_[lib_name], config, messenger, geo_manager);
        }

        // Loop through all created instantiations
        for(auto& id_mod : mod_list) {
            // FIXME: This convert the module to an unique pointer. Check that this always works and we can do this earlier
            std::unique_ptr<Module> mod(id_mod.second);
            ModuleIdentifier identifier = id_mod.first;

            // Check if the unique instantiation already exists
            auto iter = std::find_if(id_to_module_.begin(), id_to_module_.end(), [&identifier](const auto& mapv) {
                return identifier.getUniqueName() == mapv.first.getUniqueName();
            });
            if(iter != id_to_module_.end()) {
                // Unique name exists, check if its needs to be replaced
                if(identifier.getPriority() < iter->first.getPriority()) {
                    // Priority of new instance is higher, replace the instance
                    LOG(TRACE) << "Replacing model instance " << iter->first.getUniqueName()
                               << " with instance with higher priority.";

                    // Drop configuration from replaced module
                    conf_manager_->dropInstanceConfiguration(iter->first);

                    module_execution_time_.erase(iter->second->get());
                    iter->second = modules_.erase(iter->second);
                    iter = id_to_module_.erase(iter);
                } else {
                    // Priority is equal, raise an error
                    if(identifier.getPriority() == iter->first.getPriority()) {
                        throw AmbiguousInstantiationError(config.getName());
                    }
                    // Priority is lower, do not add this module to the run list, drop config
                    conf_manager_->dropInstanceConfiguration(identifier);
                    module_execution_time_.erase(id_mod.second);
                    continue;
                }
            }

            // Save the identifier in the module
            mod->set_identifier(identifier);

            // Check if module can't run in parallel
            auto module_can_parallelize = mod->multithreadingEnabled();
            if(multithreading_flag_ && !module_can_parallelize) {
                LOG(WARNING) << "Module instance " << mod->getUniqueName() << " prevents multithreading";
            }
            can_parallelize_ = module_can_parallelize && can_parallelize_;

            // Add the new module to the run list
            modules_.emplace_back(std::move(mod));
            id_to_module_[identifier] = --modules_.end();
        }
    }

    // Force MT off for all modules in case MT was not requested or some modules didn't enable multithreading
    if(!(multithreading_flag_ && can_parallelize_)) {
        for(auto& module : modules_) {
            module->set_multithreading(false);
        }
    }
    LOG_PROGRESS(STATUS, "LOAD_LOOP") << "Loaded " << configs.size() << " modules";
}

/**
 * Calls config_manager->addInstanceConfiguration(identifier, config) while handling ModuleIdentifierAlreadyAddedError
 */
static inline Configuration&
add_instance_configuration(ConfigManager* config_manager, const ModuleIdentifier& identifier, const Configuration& config) {
    try {
        return config_manager->addInstanceConfiguration(identifier, config);
    } catch(ModuleIdentifierAlreadyAddedError&) {
        throw AmbiguousInstantiationError(identifier.getUniqueName());
    }
}

/**
 * For unique modules a single instance is created per section
 */
std::pair<ModuleIdentifier, Module*> ModuleManager::create_unique_modules(void* library,
                                                                          Configuration& config,
                                                                          Messenger* messenger,
                                                                          GeometryManager* geo_manager) {
    // Make the vector to return
    const std::string& module_name = config.getName();

    // Return error if user tried to specialize the unique module:
    if(config.has("name")) {
        throw InvalidValueError(config, "name", "unique modules cannot be specialized using the \"name\" keyword.");
    }
    if(config.has("type")) {
        throw InvalidValueError(config, "type", "unique modules cannot be specialized using the \"type\" keyword.");
    }

    // Create the identifier
    std::string identifier_str;
    if(!config.get<std::string>("input").empty()) {
        identifier_str += config.get<std::string>("input");
    }
    if(!config.get<std::string>("output").empty()) {
        if(!identifier_str.empty()) {
            identifier_str += "_";
        }
        identifier_str += config.get<std::string>("output");
    }
    ModuleIdentifier identifier(module_name, std::move(identifier_str), 0);

    // Get the generator function for this module
    void* generator = dlsym(library, ALLPIX_GENERATOR_FUNCTION);
    // If the generator function was not found, throw an error
    if(generator == nullptr) {
        LOG(ERROR) << "Module library is invalid or outdated: required interface function not found!";
        throw allpix::DynamicLibraryError(module_name);
    }

    // Create and add module instance config
    Configuration& instance_config = add_instance_configuration(conf_manager_, identifier, config);

    // Specialize instance configuration
    std::filesystem::path output_dir = instance_config.get<std::string>("_global_dir");
    auto path_mod_name = identifier.getUniqueName();
    std::replace(path_mod_name.begin(), path_mod_name.end(), ':', '_');
    output_dir /= path_mod_name;

    // Convert to correct generator function
    auto module_generator = reinterpret_cast<Module* (*)(Configuration&, Messenger*, GeometryManager*)>(generator); // NOLINT

    LOG(DEBUG) << "Creating unique instantiation " << identifier.getUniqueName();

    // Get current time
    auto start = std::chrono::steady_clock::now();
    // Set module specific log settings
    auto old_settings = set_module_before(identifier.getUniqueName(), instance_config, "C:");
    // Build module
    Module* module = module_generator(instance_config, messenger, geo_manager);
    // Reset log
    set_module_after(std::move(old_settings));
    // Update execution time
    auto end = std::chrono::steady_clock::now();
    module_execution_time_[module] += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    // Set the module directory afterwards to catch invalid access in constructor
    module->get_configuration().set<std::string>("_output_dir", output_dir);

    // Store the module and return it to the Module Manager
    return std::make_pair(identifier, module);
}

/**
 * @throws InvalidModuleStateException If the module fails to forward the detector to the base class
 *
 * For detector modules multiple instantiations may be created per section. An instantiation is created for every detector if
 * no selection parameters are provided. Otherwise instantiations are created for every linked detector name and type.
 */
std::vector<std::pair<ModuleIdentifier, Module*>> ModuleManager::create_detector_modules(void* library,
                                                                                         Configuration& config,
                                                                                         Messenger* messenger,
                                                                                         GeometryManager* geo_manager) {
    const std::string& module_name = config.getName();
    LOG(DEBUG) << "Creating instantions for detector module " << module_name;

    // Create the basic identifier
    std::string identifier;
    if(!config.get<std::string>("input").empty()) {
        identifier += "_";
        identifier += config.get<std::string>("input");
    }
    if(!config.get<std::string>("output").empty()) {
        identifier += "_";
        identifier += config.get<std::string>("output");
    }

    // Open the library and get the module generator function
    void* generator = dlsym(library, ALLPIX_GENERATOR_FUNCTION);
    // If the generator function was not found, throw an error
    if(generator == nullptr) {
        LOG(ERROR) << "Module library is invalid or outdated: required interface function not found!";
        throw allpix::DynamicLibraryError(module_name);
    }
    // Convert to correct generator function
    auto module_generator =
        reinterpret_cast<Module* (*)(Configuration&, Messenger*, std::shared_ptr<Detector>)>(generator); // NOLINT

    // Handle empty type and name arrays:
    bool instances_created = false;
    std::vector<std::pair<std::shared_ptr<Detector>, ModuleIdentifier>> instantiations;

    // Create all names first with highest priority
    std::set<std::string> module_names;
    if(config.has("name")) {
        std::vector<std::string> names = config.getArray<std::string>("name");
        for(auto& name : names) {
            auto det = geo_manager->getDetector(name);
            instantiations.emplace_back(det, ModuleIdentifier(module_name, det->getName() + identifier, 0));

            // Save the name (to not instantiate it again later)
            module_names.insert(name);
        }
        instances_created = !names.empty();
    }

    // Then create all types that are not yet name instantiated
    if(config.has("type")) {
        std::vector<std::string> types = config.getArray<std::string>("type");
        for(auto& type : types) {
            auto detectors = geo_manager->getDetectorsByType(type);

            for(auto& det : detectors) {
                // Skip all that were already added by name
                if(module_names.find(det->getName()) != module_names.end()) {
                    continue;
                }

                instantiations.emplace_back(det, ModuleIdentifier(module_name, det->getName() + identifier, 1));
            }
        }
        instances_created = !types.empty();
    }

    // Create for all detectors if no name / type provided
    if(!instances_created) {
        auto detectors = geo_manager->getDetectors();

        for(auto& det : detectors) {
            instantiations.emplace_back(det, ModuleIdentifier(module_name, det->getName() + identifier, 2));
        }
    }

    // Construct instantiations from the list of requests
    std::vector<std::pair<ModuleIdentifier, Module*>> module_list;
    for(auto& instance : instantiations) {
        LOG(DEBUG) << "Creating detector instantiation " << instance.second.getUniqueName();
        // Get current time
        auto start = std::chrono::steady_clock::now();

        // Create and add module instance config
        Configuration& instance_config = add_instance_configuration(conf_manager_, instance.second, config);

        // Add internal module config
        std::filesystem::path output_dir = instance_config.get<std::string>("_global_dir");
        auto path_mod_name = instance.second.getUniqueName();
        std::replace(path_mod_name.begin(), path_mod_name.end(), ':', '/');
        output_dir /= path_mod_name;

        // Set module specific log settings
        auto old_settings = set_module_before(instance.second.getUniqueName(), instance_config, "C:");
        // Build module
        Module* module = module_generator(instance_config, messenger, instance.first);
        // Reset logging
        set_module_after(std::move(old_settings));
        // Update execution time
        auto end = std::chrono::steady_clock::now();
        module_execution_time_[module] += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        // Set the module directory afterwards to catch invalid access in constructor
        module->get_configuration().set<std::string>("_output_dir", output_dir);

        // Check if the module called the correct base class constructor
        if(module->getDetector().get() != instance.first.get()) {
            throw InvalidModuleStateException(
                "Module " + module_name +
                " does not call the correct base Module constructor: the provided detector should be forwarded");
        }

        // Store the module
        module_list.emplace_back(instance.second, module);
    }

    return module_list;
}

// Helper functions to set the module specific log settings if necessary
std::tuple<LogLevel, LogFormat, std::string, uint64_t> ModuleManager::set_module_before(const std::string& name,
                                                                                        const Configuration& config,
                                                                                        const std::string& prefix,
                                                                                        const uint64_t event) {
    // Set new log level if necessary
    LogLevel prev_level = Log::getReportingLevel();
    if(config.has("log_level")) {
        auto log_level_string = config.get<std::string>("log_level");
        std::transform(log_level_string.begin(), log_level_string.end(), log_level_string.begin(), ::toupper);
        try {
            LogLevel log_level = Log::getLevelFromString(log_level_string);
            if(log_level != prev_level) {
                LOG(TRACE) << "Local log level is set to " << log_level_string;
                Log::setReportingLevel(log_level);
            }
        } catch(std::invalid_argument& e) {
            throw InvalidValueError(config, "log_level", e.what());
        }
    }

    // Set new log format if necessary
    LogFormat prev_format = Log::getFormat();
    if(config.has("log_format")) {
        auto log_format_string = config.get<std::string>("log_format");
        std::transform(log_format_string.begin(), log_format_string.end(), log_format_string.begin(), ::toupper);
        try {
            LogFormat log_format = Log::getFormatFromString(log_format_string);
            if(log_format != prev_format) {
                LOG(TRACE) << "Local log format is set to " << log_format_string;
                Log::setFormat(log_format);
            }
        } catch(std::invalid_argument& e) {
            throw InvalidValueError(config, "log_format", e.what());
        }
    }

    // Set new section name
    auto prev_section = Log::getSection();
    Log::setSection(prefix + name);

    // Set new event number:
    auto prev_event = Log::getEventNum();
    Log::setEventNum(event);

    return std::make_tuple(prev_level, prev_format, prev_section, prev_event);
}

void ModuleManager::set_module_after(std::tuple<LogLevel, LogFormat, std::string, uint64_t> prev) {
    // Reset the previous log level
    LogLevel cur_level = Log::getReportingLevel();
    LogLevel old_level = std::get<0>(prev);
    if(cur_level != old_level) {
        Log::setReportingLevel(old_level);
        LOG(TRACE) << "Reset log level to global level of " << Log::getStringFromLevel(old_level);
    }

    // Reset the previous log format
    LogFormat cur_format = Log::getFormat();
    LogFormat old_format = std::get<1>(prev);
    if(cur_format != old_format) {
        Log::setFormat(old_format);
        LOG(TRACE) << "Reset log format to global level of " << Log::getStringFromFormat(old_format);
    }

    // Reset section name
    Log::setSection(std::get<2>(prev));

    // Reset event number
    Log::setEventNum(std::get<3>(prev));
}

/**
 * Sets the section header and logging settings before executing the  \ref Module::initialize() function.
 */
void ModuleManager::initialize() {

    Configuration& global_config = conf_manager_->getGlobalConfiguration();
    LOG(TRACE) << "Register number of workers for possible multithreading";
    if(multithreading_flag_ && can_parallelize_) {
        // Try to fetch a suitable number of workers if multithreading is enabled
        auto available_hardware_concurrency = std::thread::hardware_concurrency();
        if(available_hardware_concurrency > 2u) {
            // Try to be graceful and leave one core out if the number of workers was not specified
            available_hardware_concurrency -= 1u;
        }
        number_of_threads_ = global_config.get<unsigned int>("workers", std::max(available_hardware_concurrency, 1u));
        if(number_of_threads_ < 1) {
            throw InvalidValueError(global_config, "workers", "number of workers should be larger than zero");
        } else if(number_of_threads_ == 1) {
            LOG(WARNING) << "Using multithreading with only one worker, this might be slower than multithreading=false";
        }

        if(number_of_threads_ > std::thread::hardware_concurrency()) {
            LOG(WARNING) << "Using more workers (" << number_of_threads_
                         << ") than supported concurrent threads on this system (" << std::thread::hardware_concurrency()
                         << ") may impact simulation performance";
        }

        LOG(STATUS) << "Multithreading enabled, processing events in parallel on " << number_of_threads_
                    << " worker threads";

        // Adjust the modules buffer size according to the number of threads used
        max_buffer_size_ = global_config.get<size_t>("buffer_per_worker", 256) * number_of_threads_;
        if(max_buffer_size_ < number_of_threads_) {
            throw InvalidValueError(global_config, "buffer_per_worker", "buffer per worker should be larger than one");
        }
        LOG(STATUS) << "Allocating a total of " << max_buffer_size_ << " event slots for buffered modules";
    } else {
        // Issue a warning in case MT was requested but we can't actually run in MT
        if(multithreading_flag_ && !can_parallelize_) {
            global_config.set<bool>("multithreading", false);
            LOG(ERROR) << "Multithreading disabled since the current module configuration does not support it";
        } else {
            LOG(STATUS) << "Multithreading disabled";
        }
    }

    // Store final number of threads to the config for later reference
    global_config.set<size_t>("workers", number_of_threads_, true);

    // Initialize the thread pool with the number of threads
    if(number_of_threads_ > 0) {
        ThreadPool::registerThreadCount(number_of_threads_);
    }

    // Book global performance histograms
    if(global_config.get<bool>("performance_plots")) {
        buffer_fill_level_ = CreateHistogram<TH1D>("buffer_fill_level",
                                                   "Buffer fill level;# buffered events;# events",
                                                   static_cast<int>(max_buffer_size_),
                                                   0,
                                                   static_cast<double>(max_buffer_size_));
        event_time_ = CreateHistogram<TH1D>("event_time", "processing time per event;time [s];# events", 1000, 0, 10);
    }

    auto start_time = std::chrono::steady_clock::now();
    LOG_PROGRESS(STATUS, "INIT_LOOP") << "Initializing " << modules_.size() << " module instantiations";
    for(auto& module : modules_) {
        LOG_PROGRESS(TRACE, "INIT_LOOP") << "Initializing " << module->get_identifier().getUniqueName();

        // Pass the config manager to this instance
        module->set_config_manager(conf_manager_);

        // Create main ROOT directory for this module class if it does not exists yet
        LOG(TRACE) << "Creating and accessing ROOT directory";
        std::string module_name = module->get_configuration().getName();
        auto* directory = modules_file_->GetDirectory(module_name.c_str());
        if(directory == nullptr) {
            directory = modules_file_->mkdir(module_name.c_str());
            if(directory == nullptr) {
                throw RuntimeError("Cannot create or access overall ROOT directory for module " + module_name);
            }
        }
        directory->cd();

        // Create local directory for this instance
        TDirectory* local_directory = nullptr;
        if(module->get_identifier().getIdentifier().empty()) {
            local_directory = directory;
        } else {
            local_directory = directory->mkdir(module->get_identifier().getIdentifier().c_str());
            if(local_directory == nullptr) {
                throw RuntimeError("Cannot create or access local ROOT directory for module " + module->getUniqueName());
            }
        }

        // Change to the directory and save it in the module
        local_directory->cd();
        module->set_ROOT_directory(local_directory);

        // Get current time
        auto start = std::chrono::steady_clock::now();
        // Set module specific settings
        auto old_settings = set_module_before(module->get_identifier().getUniqueName(), module->get_configuration(), "I:");
        // Change to our ROOT directory
        module->getROOTDirectory()->cd();
        // Init module
        module->initialize();
        // Reset logging
        set_module_after(std::move(old_settings));
        // Update execution time
        auto end = std::chrono::steady_clock::now();
        module_execution_time_[module.get()] += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        // Book per-module performance plots
        if(global_config.get<bool>("performance_plots")) {
            const auto& module_identifier = module->get_identifier();
            const auto& identifier = module_identifier.getIdentifier();
            const auto& name = (identifier.empty() ? module->get_configuration().getName() : identifier);
            auto title = module->get_configuration().getName() + " event processing time " +
                         (!identifier.empty() ? "for " + identifier : "") + ";time [s];# events";
            module_event_time_.emplace(module.get(), CreateHistogram<TH1D>(name.c_str(), title.c_str(), 1000, 0, 1));
        }
    }
    LOG_PROGRESS(STATUS, "INIT_LOOP") << "Initialized " << modules_.size() << " module instantiations";
    auto end_time = std::chrono::steady_clock::now();
    initialize_time_ =
        static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count());
}

/**
 * Initializes the thread pool and executes each event in parallel.
 */
void ModuleManager::run(RandomNumberGenerator& seeder) {
    using namespace std::chrono_literals;

    Configuration& global_config = conf_manager_->getGlobalConfiguration();
    auto plot = global_config.get<bool>("performance_plots");

    // Creates the thread pool
    LOG(TRACE) << "Initializing thread pool with " << number_of_threads_ << " threads";
    auto initialize_function =
        [log_level = Log::getReportingLevel(), log_format = Log::getFormat(), modules_list = modules_]() {
            // Initialize the threads to the same log level and format as the master setting
            Log::setReportingLevel(log_level);
            Log::setFormat(log_format);

            // Call per-thread initialization of each module
            for(const auto& module : modules_list) {
                // Set module specific log settings
                auto old_settings = ModuleManager::set_module_before(
                    module->get_identifier().getUniqueName(), module->get_configuration(), "T:");

                LOG(TRACE) << "Initializing thread " << std::this_thread::get_id();
                module->initializeThread();

                // Reset logging
                ModuleManager::set_module_after(std::move(old_settings));
            }
        };

    // Finalize modules for each thread
    auto finalize_function = [modules_list = modules_]() {
        for(const auto& module : modules_list) {
            // Set module specific log settings
            auto old_settings = ModuleManager::set_module_before(
                module->get_identifier().getUniqueName(), module->get_configuration(), "T:");

            LOG(TRACE) << "Finalizing thread " << std::this_thread::get_id();
            module->finalizeThread();

            // Reset logging
            ModuleManager::set_module_after(std::move(old_settings));
        }
    };

    // Push 128 events for each worker to maintain enough work
    auto max_queue_size = number_of_threads_ * 128;
    thread_pool_ = std::make_unique<ThreadPool>(
        number_of_threads_, max_queue_size, max_buffer_size_, initialize_function, finalize_function);

    // Record the run stage total time
    auto start_time = std::chrono::steady_clock::now();

    // Push all events to the thread pool
    std::atomic<uint64_t> finished_events{0};
    std::atomic<uint64_t> aborted_events{0};
    global_config.setDefault<uint64_t>("number_of_events", 1u);
    auto number_of_events = global_config.get<uint64_t>("number_of_events");

    // Skip first N events and discard their event seed from the seeder engine:
    auto skip_events = global_config.get<uint64_t>("skip_events", 0);
    seeder.discard(skip_events);

    // Mark the first N events as completed for the thread pool. Since events start at one, always mark zero identifier as
    // completed
    for(size_t n = 0; n <= skip_events; n++) {
        thread_pool_->markComplete(n);
    }

    LOG(STATUS) << "Starting event loop";
    for(uint64_t i = 1 + skip_events; i <= number_of_events + skip_events; i++) {
        // Check if run was aborted and stop pushing extra events to the threadpool
        if(terminate_) {
            LOG(INFO) << "Interrupting event loop after " << finished_events << " events because of request to terminate";
            thread_pool_->destroy();
            break;
        }

        // Get a new seed for the new event
        uint64_t seed = seeder();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
        auto event_function_with_module =
            [this, plot, number_of_events, event_num = i, event_seed = seed, &finished_events, &aborted_events](
                std::shared_ptr<Event> event,
                ModuleList::iterator module_iter,
                int64_t event_time,
                auto&& self_func) mutable -> void {
            // The RNG to be used by all events running on this thread
            static thread_local RandomNumberGenerator random_engine;

            // Create the event data
            if(event == nullptr) {
                event = std::make_shared<Event>(*this->messenger_, event_num, event_seed);
                event->set_and_seed_random_engine(&random_engine);
                LOG(INFO) << "Starting event " << event_num << " with seed " << event_seed;
            } else {
                LOG(TRACE) << "Continue with earlier event, restoring random seed";
                event->set_and_seed_random_engine(&random_engine);
                event->restore_random_engine_state();
            }

            while(module_iter != modules_.end()) {
                auto module = *module_iter;

                LOG_PROGRESS(TRACE, "EVENT_LOOP")
                    << "Running event " << event->number << " [" << module->get_identifier().getUniqueName() << "]";

                // Check if the module is satisfied to run
                if(!module->check_delegates(this->messenger_, event.get())) {
                    LOG(TRACE) << "Not all required messages are received for " << module->get_identifier().getUniqueName()
                               << ", skipping module!";
                    ++module_iter;
                    continue;
                }

                // Get current time
                auto start = std::chrono::steady_clock::now();

                // Set module specific logging settings
                auto old_settings = ModuleManager::set_module_before(
                    module->get_identifier().getUniqueName(), module->get_configuration(), "R:", event->number);

                // Run module
                bool stop = false;
                bool abort = false;
                try {
                    if(module->require_sequence() && event_num != thread_pool_->minimumUncompleted()) {
                        stop = true;
                    } else {
                        module->run(event.get());
                    }
                } catch(const MissingDependenciesException& e) {
                    stop = true;
                } catch(const AbortEventException& e) {
                    LOG(WARNING) << "Event aborted:" << std::endl << e.what();
                    abort = true;
                } catch(const EndOfRunException& e) {
                    // Terminate if the module threw the EndOfRun request exception:
                    LOG(WARNING) << "Request to terminate:" << std::endl << e.what();
                    this->terminate_ = true;
                }

                // Reset logging
                ModuleManager::set_module_after(std::move(old_settings));

                // Update execution time
                auto end = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                // Note: we do not need to lock a mutex because the std::map is not altered and its values are atomic.
                this->module_execution_time_[module.get()] += duration;

                if(plot) {
                    std::lock_guard<std::mutex> stat_lock{event->stats_mutex_};
                    event_time += duration;
                    this->module_event_time_[module.get()]->Fill(
                        std::chrono::duration<double>(std::chrono::nanoseconds(duration)).count());
                }

                if(abort) {
                    // Break module execution loop:
                    aborted_events++;
                    break;
                }

                if(stop) {
                    LOG(DEBUG) << "Event " << event->number
                               << " was interrupted because of missing dependencies, rescheduling...";
                    // Store state of PRNG engine:
                    event->store_random_engine_state();
                    // Reschedule the event:
                    auto event_function = std::bind(self_func, event, module_iter, event_time, self_func);
                    auto future = thread_pool_->submit(event->number, event_function, false);
                    assert(future.valid() || !thread_pool_->valid());
                    auto buffered_events = thread_pool_->bufferedQueueSize();
                    LOG_PROGRESS(STATUS, "EVENT_LOOP") << "Buffered " << buffered_events << ", finished " << finished_events
                                                       << " of " << number_of_events << " events";
                    return;
                }

                ++module_iter;
            }
#pragma GCC diagnostic pop

            // All modules finished, mark as complete
            thread_pool_->markComplete(event->number);
            LOG(INFO) << "Finished event " << event_num << " with seed " << event_seed;

            auto buffered_events = thread_pool_->bufferedQueueSize();
            if(plot) {
                this->buffer_fill_level_->Fill(static_cast<double>(buffered_events));
                event_time_->Fill(static_cast<double>(event_time) * 1e-9);
            }

            finished_events++;
            LOG_PROGRESS(STATUS, "EVENT_LOOP") << "Buffered " << buffered_events << ", finished " << finished_events
                                               << " of " << number_of_events << " events";
        };

        auto event_function =
            std::bind(event_function_with_module, nullptr, modules_.begin(), 0, event_function_with_module);

        auto future = thread_pool_->submit(event_function);
        assert(future.valid() || !thread_pool_->valid());
        thread_pool_->checkException();
    }

    LOG(TRACE) << "All events have been initialized. Waiting for thread pool to finish...";

    // Wait for workers to finish
    thread_pool_->wait();

    // Check exception for last events
    thread_pool_->checkException();

    LOG_PROGRESS(STATUS, "EVENT_LOOP") << "Finished run of " << finished_events << " events";
    global_config.set<uint64_t>("number_of_events", finished_events);

    if(aborted_events > 0) {
        LOG(WARNING) << "Aborted " << aborted_events << " events in this run";
    }

    auto end_time = std::chrono::steady_clock::now();
    run_time_ = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count());

    LOG(TRACE) << "Destroying thread pool";
    thread_pool_.reset();
}

static std::string nanoseconds_to_time(uint64_t nanoseconds) {
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::nanoseconds(nanoseconds));

    std::string time_str;
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
    duration -= hours;
    if(hours.count() > 0) {
        time_str += std::to_string(hours.count());
        time_str += " hours ";
    }
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
    duration -= minutes;
    if(minutes.count() > 0) {
        time_str += std::to_string(minutes.count());
        time_str += " minutes ";
    }
    time_str += std::to_string(duration.count());
    time_str += " seconds";

    return time_str;
}

/**
 * Sets the section header and logging settings before executing the  \ref Module::finalize() function. Reset the logging
 * after finalization. No method will be called after finalizing the module (except the destructor).
 */
void ModuleManager::finalize() {
    auto start_time = std::chrono::steady_clock::now();
    LOG_PROGRESS(TRACE, "FINALIZE_LOOP") << "Finalizing module instantiations";
    for(auto& module : modules_) {
        LOG_PROGRESS(TRACE, "FINALIZE_LOOP") << "Finalizing " << module->get_identifier().getUniqueName();

        // Get current time
        auto start = std::chrono::steady_clock::now();

        // Set module specific log settings
        auto old_settings = set_module_before(module->get_identifier().getUniqueName(), module->get_configuration(), "F:");
        // Change to our ROOT directory
        module->getROOTDirectory()->cd();
        // Finalize module
        module->finalize();
        // Remove the pointer to the ROOT directory after finalizing
        module->set_ROOT_directory(nullptr);
        // Remove the config manager
        module->set_config_manager(nullptr);
        set_module_after(std::move(old_settings));
        // Update execution time
        auto end = std::chrono::steady_clock::now();
        module_execution_time_[module.get()] += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    }

    // Store performance plots
    Configuration& global_config = conf_manager_->getGlobalConfiguration();
    if(global_config.get<bool>("performance_plots")) {

        auto* perf_dir = modules_file_->mkdir("performance");
        if(perf_dir == nullptr) {
            throw RuntimeError("Cannot create or access ROOT directory for performance plots");
        }
        perf_dir->cd();

        event_time_->Write();
        buffer_fill_level_->Write();

        for(auto& module : modules_) {
            const auto& module_name = module->get_configuration().getName();
            auto* mod_dir = perf_dir->GetDirectory(module_name.c_str());
            if(mod_dir == nullptr) {
                mod_dir = perf_dir->mkdir(module_name.c_str());
                if(mod_dir == nullptr) {
                    throw RuntimeError("Cannot create or access ROOT directory for performance plots of module " +
                                       module_name);
                }
            }
            mod_dir->cd();

            // Write the histogram
            module_event_time_[module.get()]->Write();
        }
    }

    // Close module ROOT file
    modules_file_->Close();
    LOG_PROGRESS(STATUS, "FINALIZE_LOOP") << "Finalization completed";
    auto end_time = std::chrono::steady_clock::now();
    finalize_time_ =
        static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count());
    auto total_time = initialize_time_ + run_time_ + finalize_time_;

    // Check for unused configuration keys:
    auto unused_keys = global_config.getUnusedKeys();
    if(!unused_keys.empty()) {
        std::stringstream st;
        st << "Unused configuration keys in global section:";
        for(auto& key : unused_keys) {
            st << std::endl << key;
        }
        LOG(WARNING) << st.str();
    }
    for(const auto& config : conf_manager_->getInstanceConfigurations()) {
        auto unique_name = config.getName();
        auto identifier = config.get<std::string>("identifier");
        if(!identifier.empty()) {
            unique_name += ":";
            unique_name += identifier;
        }
        auto cfg_unused_keys = config.getUnusedKeys();
        if(!cfg_unused_keys.empty()) {
            std::stringstream st;
            st << "Unused configuration keys in section " << unique_name << ":";
            for(auto& key : cfg_unused_keys) {
                st << std::endl << key;
            }
            LOG(WARNING) << st.str();
        }
    }

    // Find the slowest module, and accumulate the total run-time for all modules
    int64_t slowest_time = 0, total_module_time = 0;
    std::string slowest_module;
    for(auto& module_exec_time : module_execution_time_) {
        total_module_time += module_exec_time.second;
        if(module_exec_time.second > slowest_time) {
            slowest_time = module_exec_time.second;
            slowest_module = module_exec_time.first->getUniqueName();
        }
    }
    LOG(STATUS) << "Executed " << modules_.size() << " instantiations in " << nanoseconds_to_time(total_time)
                << ", spending " << std::round((100 * slowest_time) / std::max(int64_t(1), total_module_time))
                << "% of time in slowest instantiation " << slowest_module;
    for(auto& module : modules_) {
        LOG(INFO) << " Module " << module->getUniqueName() << " took "
                  << Units::display(module_execution_time_[module.get()].load(), {"s", "ms"});
    }

    auto processing_time = std::round(run_time_ / std::max(uint64_t(1), global_config.get<uint64_t>("number_of_events")));
    LOG(STATUS) << "Average processing time is \x1B[1m" << Units::display(processing_time, {"ms", "us"})
                << "/event\x1B[0m, event generation at \x1B[1m"
                << std::round(global_config.get<double>("number_of_events") / Units::convert(run_time_, "s"))
                << " Hz\x1B[0m";

    if(global_config.get<unsigned int>("workers") > 0) {
        auto event_processing_time = std::round(processing_time * global_config.get<unsigned int>("workers"));
        LOG(STATUS) << "This corresponds to a processing time of \x1B[1m"
                    << Units::display(event_processing_time, {"ms", "us"}) << "/event\x1B[0m per worker";
    }
}

/**
 * All modules in the event loop continue to finish the current event
 */
void ModuleManager::terminate() {
    if(!terminate_.exchange(true) && thread_pool_) {
        thread_pool_->destroy();
    }
}
