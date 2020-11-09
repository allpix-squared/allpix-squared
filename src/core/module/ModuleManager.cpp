/**
 * @file
 * @brief Implementation of module manager
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "ModuleManager.hpp"
#include "Event.hpp"

#include <dlfcn.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <limits>
#include <random>
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
#include "core/utils/file.h"
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
    global_config.setDefault("multithreading", false);
    multithreading_flag_ = global_config.get<bool>("multithreading");

    // Set default for performance plot creation:
    global_config.setDefault("performance_plots", false);

    // Store the messenger
    messenger_ = messenger;

    // (Re)create the main ROOT file
    auto path = std::string(gSystem->pwd()) + "/" + global_config.get<std::string>("root_file", "modules");
    path = allpix::add_file_extension(path, "root");

    if(allpix::path_is_file(path)) {
        if(global_config.get<bool>("deny_overwrite", false)) {
            throw RuntimeError("Overwriting of existing main ROOT file " + path + " denied");
        }
        LOG(WARNING) << "Main ROOT file " << path << " exists and will be overwritten.";
        allpix::remove_file(path);
    }
    modules_file_ = std::make_unique<TFile>(path.c_str(), "RECREATE");
    if(modules_file_->IsZombie()) {
        throw RuntimeError("Cannot create main ROOT file " + path);
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
                std::vector<std::string> lib_paths = global_config.getPathArray("library_directories", true);
                for(auto& lib_path : lib_paths) {
                    std::string full_lib_path = lib_path;
                    full_lib_path += "/";
                    full_lib_path += lib_name;

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
            auto iter = id_to_module_.find(identifier);
            if(iter != id_to_module_.end()) {
                // Unique name exists, check if its needs to be replaced
                if(identifier.getPriority() < iter->first.getPriority()) {
                    // Priority of new instance is higher, replace the instance
                    LOG(TRACE) << "Replacing model instance " << iter->first.getUniqueName()
                               << " with instance with higher priority.";

                    module_execution_time_.erase(iter->second->get());
                    iter->second = modules_.erase(iter->second);
                    iter = id_to_module_.erase(iter);
                } else {
                    // Priority is equal, raise an error
                    if(identifier.getPriority() == iter->first.getPriority()) {
                        throw AmbiguousInstantiationError(config.getName());
                    }
                    // Priority is lower, do not add this module to the run list
                    continue;
                }
            }

            // Save the identifier in the module
            mod->set_identifier(identifier);

            // Check if module can't run in parallel
            can_parallelize_ = mod->canParallelize() && can_parallelize_;

            // Add the new module to the run list
            modules_.emplace_back(std::move(mod));
            id_to_module_[identifier] = --modules_.end();
        }
    }

    // Force MT off for all modules in case MT was not requested or some modules didn't enable parallelization
    if(!(multithreading_flag_ && can_parallelize_)) {
        for(auto& module : modules_) {
            module->set_parallelize(false);
        }
    }
    LOG_PROGRESS(STATUS, "LOAD_LOOP") << "Loaded " << configs.size() << " modules";
}

/**
 * For unique modules a single instance is created per section
 */
std::pair<ModuleIdentifier, Module*> ModuleManager::create_unique_modules(void* library,
                                                                          Configuration& config,
                                                                          Messenger* messenger,
                                                                          GeometryManager* geo_manager) {
    // Make the vector to return
    std::string module_name = config.getName();

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
    ModuleIdentifier identifier(module_name, identifier_str, 0);

    // Get the generator function for this module
    void* generator = dlsym(library, ALLPIX_GENERATOR_FUNCTION);
    // If the generator function was not found, throw an error
    if(generator == nullptr) {
        LOG(ERROR) << "Module library is invalid or outdated: required interface function not found!";
        throw allpix::DynamicLibraryError(module_name);
    }

    // Create and add module instance config
    Configuration& instance_config = conf_manager_->addInstanceConfiguration(identifier, config);

    // Specialize instance configuration
    std::string output_dir;
    output_dir = instance_config.get<std::string>("_global_dir");
    output_dir += "/";
    std::string path_mod_name = identifier.getUniqueName();
    std::replace(path_mod_name.begin(), path_mod_name.end(), ':', '_');
    output_dir += path_mod_name;

    // Convert to correct generator function
    auto module_generator = reinterpret_cast<Module* (*)(Configuration&, Messenger*, GeometryManager*)>(generator); // NOLINT

    LOG(DEBUG) << "Creating unique instantiation " << identifier.getUniqueName();

    // Get current time
    auto start = std::chrono::steady_clock::now();
    // Set the log section header
    std::string old_section_name = Log::getSection();
    std::string section_name = "C:";
    section_name += identifier.getUniqueName();
    Log::setSection(section_name);
    // Set module specific log settings
    auto old_settings = set_module_before(identifier.getUniqueName(), instance_config);
    // Build module
    Module* module = module_generator(instance_config, messenger, geo_manager);
    // Reset log
    Log::setSection(old_section_name);
    set_module_after(old_settings);
    // Update execution time
    auto end = std::chrono::steady_clock::now();
    module_execution_time_[module] += static_cast<std::chrono::duration<long double>>(end - start).count();

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
    std::string module_name = config.getName();
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
        Configuration& instance_config = conf_manager_->addInstanceConfiguration(instance.second, config);

        // Add internal module config
        std::string output_dir;
        output_dir = instance_config.get<std::string>("_global_dir");
        output_dir += "/";
        std::string path_mod_name = instance.second.getUniqueName();
        std::replace(path_mod_name.begin(), path_mod_name.end(), ':', '/');
        output_dir += path_mod_name;

        // Set the log section header
        std::string old_section_name = Log::getSection();
        std::string section_name = "C:";
        section_name += instance.second.getUniqueName();
        Log::setSection(section_name);
        // Set module specific log settings
        auto old_settings = set_module_before(instance.second.getUniqueName(), instance_config);
        // Build module
        Module* module = module_generator(instance_config, messenger, instance.first);
        // Reset logging
        Log::setSection(old_section_name);
        set_module_after(old_settings);
        // Update execution time
        auto end = std::chrono::steady_clock::now();
        module_execution_time_[module] += static_cast<std::chrono::duration<long double>>(end - start).count();

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
std::tuple<LogLevel, LogFormat> ModuleManager::set_module_before(const std::string&, const Configuration& config) {
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

    return std::make_tuple(prev_level, prev_format);
}
void ModuleManager::set_module_after(std::tuple<LogLevel, LogFormat> prev) {
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
}

/**
 * Sets the section header and logging settings before executing the  \ref Module::init() function.
 *  \ref Module::reset_delegates() "Resets" the delegates and the logging after initialization.
 */
void ModuleManager::init(RandomNumberGenerator& seeder) {
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

        // Set the RNG to be used by the module initialization
        module->set_random_generator(&seeder);

        // Get current time
        auto start = std::chrono::steady_clock::now();
        // Set init module section header
        std::string old_section_name = Log::getSection();
        std::string section_name = "I:";
        section_name += module->get_identifier().getUniqueName();
        Log::setSection(section_name);
        // Set module specific settings
        auto old_settings = set_module_before(module->get_identifier().getUniqueName(), module->get_configuration());
        // Change to our ROOT directory
        module->getROOTDirectory()->cd();
        // Init module
        module->init();
        // Reset logging
        Log::setSection(old_section_name);
        set_module_after(old_settings);
        // Update execution time
        auto end = std::chrono::steady_clock::now();
        module_execution_time_[module.get()] += static_cast<std::chrono::duration<long double>>(end - start).count();

        // Reset the random number generator for this module
        module->set_random_generator(nullptr);
    }
    LOG_PROGRESS(STATUS, "INIT_LOOP") << "Initialized " << modules_.size() << " module instantiations";
    auto end_time = std::chrono::steady_clock::now();
    total_time_ += static_cast<std::chrono::duration<long double>>(end_time - start_time).count();
}

/**
 * Initializes the thread pool and executes each event in parallel.
 */
void ModuleManager::run(RandomNumberGenerator& seeder) {
    using namespace std::chrono_literals;

    Configuration& global_config = conf_manager_->getGlobalConfiguration();
    auto plot = global_config.get<bool>("performance_plots");

    // Default to no additional thread without multithreading
    size_t threads_num = 0;

    // See if we can run in parallel with how many workers
    if(multithreading_flag_ && can_parallelize_) {
        // Try to fetch a suitable number of workers if multithreading is enabled
        auto available_hardware_concurrency = std::thread::hardware_concurrency();
        if(available_hardware_concurrency > 0u) {
            // Try to be graceful and leave one core out if the number of workers was not specified
            available_hardware_concurrency -= 1u;
        }
        threads_num = global_config.get<unsigned int>("workers", std::max(available_hardware_concurrency, 1u));
        if(threads_num == 0) {
            throw InvalidValueError(global_config, "workers", "number of workers should be strictly more than zero");
        }
        LOG(STATUS) << "Multithreading enabled, processing events in parallel on " << threads_num << " worker threads";

        if(threads_num > std::thread::hardware_concurrency()) {
            LOG(WARNING) << "Using more workers (" << threads_num << ") than supported concurrent threads on this system ("
                         << std::thread::hardware_concurrency() << ") may impact simulation performance";
        }

        // Adjust the modules buffer size according to the number of threads used
        auto buffer_size = global_config.get<size_t>("module_buffer_depth", 128) * threads_num;
        LOG(STATUS) << "Allocating a total of " << buffer_size << " event slots for buffered modules";
        BufferedModule::set_max_buffer_size(buffer_size);
    } else {
        // Issue a warning in case MT was requested but we can't actually run in MT
        if(multithreading_flag_ && !can_parallelize_) {
            global_config.set<bool>("multithreading", false);
            LOG(WARNING) << "Multithreading disabled since the current module configuration doesn't support it";
        }
    }
    global_config.set<size_t>("workers", threads_num);

    // Book performance histograms
    if(global_config.get<bool>("performance_plots")) {
        buffer_fill_level_ = CreateHistogram<TH1D>(
            "buffer_fill_level", "Buffer fill level;# buffered events;# events", 128 * threads_num, 0, 128 * threads_num);
        event_time_ = CreateHistogram<TH1D>("event_time", "processing time per event;time [s];# events", 1000, 0, 10);
        for(auto& module : modules_) {
            auto identifier = module->get_identifier().getIdentifier();
            auto name = (identifier.empty() ? module->get_configuration().getName() : identifier);
            auto title = module->get_configuration().getName() + " event processing time " +
                         (!identifier.empty() ? "for " + identifier : "") + ";time [s];# events";
            module_event_time_.emplace(module.get(), CreateHistogram<TH1D>(name.c_str(), title.c_str(), 1000, 0, 1));
        }
    }

    // Creates the thread pool
    LOG(TRACE) << "Initializing thread pool with " << threads_num << " thread";
    auto initialize_function =
        [log_level = Log::getReportingLevel(), log_format = Log::getFormat(), modules_list = modules_]() {
            // Initialize the threads to the same log level and format as the master setting
            Log::setReportingLevel(log_level);
            Log::setFormat(log_format);

            // Call per-thread initialization of each module
            for(const auto& module : modules_list) {
                // Set run module section header
                std::string old_section_name = Log::getSection();
                std::string section_name = "T:";
                section_name += module->get_identifier().getUniqueName();
                Log::setSection(section_name);

                // Set module specific settings
                auto old_settings =
                    ModuleManager::set_module_before(module->get_identifier().getUniqueName(), module->get_configuration());

                LOG(TRACE) << "Initializing thread " << std::this_thread::get_id();
                module->initializeThread();

                // Reset logging
                Log::setSection(old_section_name);
                ModuleManager::set_module_after(old_settings);
            }
        };

    // Finalize modules for each thread
    auto finalize_function = [modules_list = modules_]() {
        for(const auto& module : modules_list) {
            // Set run module section header
            std::string old_section_name = Log::getSection();
            std::string section_name = "T:";
            section_name += module->get_identifier().getUniqueName();
            Log::setSection(section_name);

            // Set module specific settings
            auto old_settings =
                ModuleManager::set_module_before(module->get_identifier().getUniqueName(), module->get_configuration());

            LOG(TRACE) << "Finalizing thread " << std::this_thread::get_id();
            module->finalizeThread();

            // Reset logging
            Log::setSection(old_section_name);
            ModuleManager::set_module_after(old_settings);
        }
    };

    // Push 128 events for each worker to maintain enough work
    auto max_queue_size = threads_num * 128;
    std::unique_ptr<ThreadPool> thread_pool =
        std::make_unique<ThreadPool>(threads_num, max_queue_size, initialize_function, finalize_function);

    // Record the run stage total time
    auto start_time = std::chrono::steady_clock::now();

    // Push all events to the thread pool
    std::atomic<uint64_t> dispatched_events{0};
    global_config.setDefault<uint64_t>("number_of_events", 1u);
    auto number_of_events = global_config.get<uint64_t>("number_of_events");
    for(uint64_t i = 1; i <= number_of_events; i++) {
        // Check if run was aborted and stop pushing extra events to the threadpool
        if(terminate_) {
            LOG(INFO) << "Interrupting event loop after " << i << " events because of request to terminate";
            thread_pool->destroy();
            global_config.set<uint64_t>("number_of_events", dispatched_events);
            break;
        }

        // Get a new seed for the new event
        uint64_t seed = seeder();

        auto event_function =
            [this, plot, number_of_events, event_num = i, event_seed = seed, &dispatched_events]() mutable {
                // The RNG to be used by all events running on this thread
                static thread_local RandomNumberGenerator random_engine;

                // Create the event data
                std::shared_ptr<Event> event = std::make_shared<Event>(*this->messenger_, event_num, event_seed);
                event->set_and_seed_random_engine(&random_engine);

                long double event_time = 0;
                size_t buffered_events = 0;
                for(auto& module : modules_) {
                    LOG_PROGRESS(TRACE, "EVENT_LOOP")
                        << "Running event " << event->number << " [" << module->get_identifier().getUniqueName() << "]";

                    // Check if the module is satisfied to run
                    if(!module->check_delegates(this->messenger_, event.get())) {
                        LOG(TRACE) << "Not all required messages are received for "
                                   << module->get_identifier().getUniqueName() << ", skipping module!";
                        module->skip_event(event->number);
                        continue;
                    }

                    // Get current time
                    auto start = std::chrono::steady_clock::now();

                    // Set run module section header
                    std::string old_section_name = Log::getSection();
                    uint64_t old_event_num = Log::getEventNum();
                    std::string section_name = "R:";
                    section_name += module->get_identifier().getUniqueName();
                    Log::setSection(section_name);
                    Log::setEventNum(event->number);

                    // Set module specific settings
                    auto old_settings = ModuleManager::set_module_before(module->get_identifier().getUniqueName(),
                                                                         module->get_configuration());

                    // Run module
                    try {
                        if(module->is_buffered()) {
                            auto* buffered_module = static_cast<BufferedModule*>(module.get());
                            buffered_events += buffered_module->run_in_order(event);
                        } else {
                            module->run(event.get());
                        }
                    } catch(const EndOfRunException& e) {
                        // Terminate if the module threw the EndOfRun request exception:
                        LOG(WARNING) << "Request to terminate:" << std::endl << e.what();
                        this->terminate_ = true;
                    }

                    // Reset logging
                    Log::setSection(old_section_name);
                    Log::setEventNum(old_event_num);
                    ModuleManager::set_module_after(old_settings);

                    // Update execution time
                    auto end = std::chrono::steady_clock::now();
                    std::lock_guard<std::mutex> stat_lock{event->stats_mutex_};

                    auto duration = static_cast<std::chrono::duration<long double>>(end - start).count();
                    event_time += duration;
                    this->module_execution_time_[module.get()] += duration;

                    if(plot) {
                        this->module_event_time_[module.get()]->Fill(static_cast<double>(duration));
                    }
                }

                if(plot) {
                    this->buffer_fill_level_->Fill(static_cast<double>(buffered_events));
                    event_time_->Fill(static_cast<double>(event_time));
                }

                dispatched_events++;
                LOG_PROGRESS(STATUS, "EVENT_LOOP")
                    << "Buffered " << buffered_events << ", finished " << (dispatched_events - buffered_events) << " of "
                    << number_of_events << " events";
            };
        thread_pool->submit(event_function);
        thread_pool->checkException();
    }

    LOG(TRACE) << "All events have been initialized. Waiting for thread pool to finish...";

    // Wait for workers to finish
    thread_pool->wait();

    // Check exception for last events
    thread_pool->checkException();

    LOG_PROGRESS(STATUS, "EVENT_LOOP") << "Finished run of " << dispatched_events << " events";
    auto end_time = std::chrono::steady_clock::now();
    total_time_ += static_cast<std::chrono::duration<long double>>(end_time - start_time).count();

    LOG(TRACE) << "Destroying thread pool";
}

static std::string seconds_to_time(long double seconds) {
    auto duration = std::chrono::duration<long long>(static_cast<long long>(std::round(seconds)));

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
        // Set finalize module section header
        std::string old_section_name = Log::getSection();
        std::string section_name = "F:";
        section_name += module->get_identifier().getUniqueName();
        Log::setSection(section_name);
        // Set module specific settings
        auto old_settings = set_module_before(module->get_identifier().getUniqueName(), module->get_configuration());
        // Change to our ROOT directory
        module->getROOTDirectory()->cd();
        // Finalize module
        if(module->is_buffered()) {
            auto* buffered_module = static_cast<BufferedModule*>(module.get());
            buffered_module->finalize_buffer();
        } else {
            module->finalize();
        }
        // Remove the pointer to the ROOT directory after finalizing
        module->set_ROOT_directory(nullptr);
        // Remove the config manager
        module->set_config_manager(nullptr);
        // Reset logging
        Log::setSection(old_section_name);
        set_module_after(old_settings);
        // Update execution time
        auto end = std::chrono::steady_clock::now();
        module_execution_time_[module.get()] += static_cast<std::chrono::duration<long double>>(end - start).count();
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
            auto module_name = module->get_configuration().getName();
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
    total_time_ += static_cast<std::chrono::duration<long double>>(end_time - start_time).count();

    // Find the slowest module, and accumulate the total run-time for all modules
    long double slowest_time = 0, total_module_time = 0;
    std::string slowest_module;
    for(auto& module_time : module_execution_time_) {
        total_module_time += module_time.second;
        if(module_time.second > slowest_time) {
            slowest_time = module_time.second;
            slowest_module = module_time.first->getUniqueName();
        }
    }
    LOG(STATUS) << "Executed " << modules_.size() << " instantiations in " << seconds_to_time(total_time_) << ", spending "
                << std::round((100 * slowest_time) / std::max(1.0l, total_module_time))
                << "% of time in slowest instantiation " << slowest_module;
    for(auto& module : modules_) {
        LOG(INFO) << " Module " << module->getUniqueName() << " took " << module_execution_time_[module.get()] << " seconds";
    }

    long double processing_time = 0;
    auto total_events = global_config.get<uint64_t>("number_of_events");
    if(total_events > 0) {
        processing_time = std::round((1000 * total_time_) / total_events);
    }

    LOG(STATUS) << "Average processing time is \x1B[1m" << processing_time << " ms/event\x1B[0m, event generation at \x1B[1m"
                << std::round(global_config.get<double>("number_of_events") / total_time_) << " Hz\x1B[0m";

    if(global_config.get<unsigned int>("workers") > 0) {
        auto event_processing_time = std::round(processing_time * global_config.get<unsigned int>("workers"));
        LOG(STATUS) << "This corresponds to a processing time of \x1B[1m" << event_processing_time
                    << " ms/event\x1B[0m per worker";
    }
}

/**
 * All modules in the event loop continue to finish the current event
 */
void ModuleManager::terminate() {
    terminate_ = true;
}
