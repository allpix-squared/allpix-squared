/**
 * @file
 * @brief Implementation of module manager
 *
 * @copyright MIT License
 */

#include "ModuleManager.hpp"

#include <dlfcn.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>

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

ModuleManager::ModuleManager() = default;

/**
 * Loads the modules specified in the configuration file. Each module is contained within its own library which is loaded
 * automatically. After that the required modules are created from the configuration.
 */
void ModuleManager::load(Messenger* messenger, ConfigManager* conf_manager, GeometryManager* geo_manager) {
    std::vector<Configuration> configs = conf_manager->getConfigurations();
    global_config_ = conf_manager->getGlobalConfiguration();

    // Loop through all non-global configurations
    for(auto& config : configs) {
        // Load library for each module. Libraries are named (by convention + CMAKE) libAllpixModule Name.suffix
        std::string lib_name = std::string(ALLPIX_MODULE_PREFIX).append(config.getName()).append(SHARED_LIBRARY_SUFFIX);
        LOG(INFO) << "Loading library " << lib_name;

        void* lib = nullptr;
        bool load_error = false;
        dlerror();
        if(loaded_libraries_.count(lib_name) == 0) {
            // If library is not loaded then try to load it first from the config directories
            if(global_config_.has("library_directories")) {
                std::vector<std::string> lib_paths = global_config_.getPathArray("library_directories", true);
                for(auto& lib_path : lib_paths) {
                    std::string full_lib_path = lib_path;
                    full_lib_path += "/";
                    full_lib_path += lib_name;

                    // Check if the absolute file exists and try to load if it exists
                    std::ifstream check_file(full_lib_path);
                    if(check_file.good()) {
                        lib = dlopen(full_lib_path.c_str(), RTLD_NOW);
                        if(lib != nullptr) {
                            LOG(DEBUG) << "Found library in config specified directory at " << full_lib_path;
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
            if(lib_error != nullptr && std::strstr(lib_error, "cannot allocate memory in static TLS block") != nullptr) {
                std::string error(lib_error);
                std::string problem_lib = error.substr(0, error.find(':'));

                LOG(ERROR) << "Library could not be loaded: not enough thread local storage available" << std::endl
                           << "Try one of below workarounds:" << std::endl
                           << "- Rerun library with the environmental variable LD_PRELOAD='" << problem_lib << "'"
                           << std::endl
                           << "- Recompile the library " << problem_lib << " with tls-model=global-dynamic";
            } else {
                LOG(ERROR) << "Library could not be loaded" << std::endl
                           << " - Did you compile the library? " << std::endl
                           << " - Did you spell the library name correctly? ";
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

            // Set the configuration for this module for later use
            mod->set_configuration(config);

            // Save the identifier in the module
            mod->set_identifier(identifier);

            // Set global output directory
            std::string global_dir = gSystem->pwd();
            mod->set_global_directory(global_dir);

            // Set local module output directory
            std::string output_dir;
            output_dir = global_dir;
            output_dir += "/";
            std::string path_mod_name = identifier.getUniqueName();
            std::replace(path_mod_name.begin(), path_mod_name.end(), ':', '_');
            output_dir += path_mod_name;
            mod->set_output_directory(output_dir);

            // Add the new module to the run list
            modules_.emplace_back(std::move(mod));
            id_to_module_[identifier] = --modules_.end();
        }
    }
}

// Helper functions to set the module specific log settings if necessary
std::tuple<LogLevel, LogFormat> ModuleManager::set_module_before(const std::string&, const Configuration& config) {
    // Set new log level if necessary
    LogLevel prev_level = Log::getReportingLevel();
    if(config.has("log_level")) {
        std::string log_level_string = config.get<std::string>("log_level");
        std::transform(log_level_string.begin(), log_level_string.end(), log_level_string.begin(), ::toupper);
        try {
            LogLevel log_level = Log::getLevelFromString(log_level_string);
            if(log_level != prev_level) {
                LOG(DEBUG) << "Local log level is set to " << log_level_string;
                Log::setReportingLevel(log_level);
            }
        } catch(std::invalid_argument& e) {
            throw InvalidValueError(config, "log_level", e.what());
        }
    }

    // Set new log format if necessary
    LogFormat prev_format = Log::getFormat();
    if(config.has("log_format")) {
        std::string log_format_string = config.get<std::string>("log_format");
        std::transform(log_format_string.begin(), log_format_string.end(), log_format_string.begin(), ::toupper);
        try {
            LogFormat log_format = Log::getFormatFromString(log_format_string);
            if(log_format != prev_format) {
                LOG(DEBUG) << "Local log format is set to " << log_format_string;
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
        LOG(DEBUG) << "Reset log level to global level of " << Log::getStringFromLevel(old_level);
    }

    // Reset the previous log format
    LogFormat cur_format = Log::getFormat();
    LogFormat old_format = std::get<1>(prev);
    if(cur_format != old_format) {
        Log::setFormat(old_format);
        LOG(DEBUG) << "Reset log format to global level of " << Log::getStringFromFormat(old_format);
    }
}

/**
 * Sets the section header and logging settings before executing the  \ref Module::init() function. \ref
 * Module::reset_delegates() "Resets" the delegates and the logging after initialization.
 */
void ModuleManager::init() {
    for(auto& mod : modules_) {
        LOG_PROGRESS(QUIET, "INIT_LOOP") << "Initializing " << mod->get_identifier().getUniqueName();
        // Set init module section header
        std::string old_section_name = Log::getSection();
        std::string section_name = "I:";
        section_name += mod->get_identifier().getUniqueName();
        Log::setSection(section_name);
        // Set module specific settings
        auto old_settings = set_module_before(mod->get_identifier().getUniqueName(), mod->get_configuration());
        // Init module
        mod->init();
        // Reset delegates
        LOG(DEBUG) << "Resetting delegates";
        mod->reset_delegates();
        // Reset logging
        Log::setSection(old_section_name);
        set_module_after(old_settings);
    }
    LOG_PROGRESS(QUIET, "INIT_LOOP") << "Initialization finished";
}

/**
 * The run for a module is skipped if its delegates are not \ref Module::check_delegates() "satisfied". Sets the section
 * header and logging settings before executing the \ref Module::run() function. \ref Module::reset_delegates() "Resets" the
 * delegates and the logging after initialization
 */
void ModuleManager::run() {
    // Loop over the number of events
    auto number_of_events = global_config_.get<unsigned int>("number_of_events", 1u);
    for(unsigned int i = 0; i < number_of_events; ++i) {
        LOG_PROGRESS(QUIET, "EVENT_LOOP") << "Running event " << (i + 1) << " of " << number_of_events;
        for(auto& mod : modules_) {
            // Check if module is satisfied to run
            if(!mod->check_delegates()) {
                LOG(DEBUG) << "Not all required messages are received for " << mod->get_identifier().getUniqueName()
                           << ", skipping module!";
                continue;
            }

            // Set run module section header
            std::string old_section_name = Log::getSection();
            std::string section_name = "R:";
            section_name += mod->get_identifier().getUniqueName();
            Log::setSection(section_name);
            // Set module specific settings
            auto old_settings = set_module_before(mod->get_identifier().getUniqueName(), mod->get_configuration());
            // Run module
            mod->run(i + 1);
            // Resetting delegates
            LOG(DEBUG) << "Resetting delegates";
            mod->reset_delegates();
            // Reset logging
            Log::setSection(old_section_name);
            set_module_after(old_settings);
        }
    }
}

/**
 * Sets the section header and logging settings before executing the  \ref Module::finalize() function. Reset the logging
 * after finalization. No method will be called after finalizing the module (except the destructor).
 */
void ModuleManager::finalize() {
    for(auto& mod : modules_) {
        LOG_PROGRESS(QUIET, "INIT_LOOP") << "Finalizing " << mod->get_identifier().getUniqueName();

        // Set finalize module section header
        std::string old_section_name = Log::getSection();
        std::string section_name = "F:";
        section_name += mod->get_identifier().getUniqueName();
        Log::setSection(section_name);
        // Set module specific settings
        auto old_settings = set_module_before(mod->get_identifier().getUniqueName(), mod->get_configuration());
        // Finalize module
        mod->finalize();
        // Reset logging
        Log::setSection(old_section_name);
        set_module_after(old_settings);
    }
    LOG_PROGRESS(QUIET, "INIT_LOOP") << "Finalization completed";
}

/**
 * For unique modules a single instance is created per section
 */
std::pair<ModuleIdentifier, Module*> ModuleManager::create_unique_modules(void* library,
                                                                          const Configuration& config,
                                                                          Messenger* messenger,
                                                                          GeometryManager* geo_manager) {
    // Make the vector to return
    std::string module_name = config.getName();

    LOG(DEBUG) << "Creating instantions for unique module " << module_name;

    // Load an instance of the module from the library
    ModuleIdentifier identifier(module_name, "", 0);

    // Get the generator function for this module
    void* generator = dlsym(library, ALLPIX_GENERATOR_FUNCTION);
    // If the generator function was not found, throw an error
    if(generator == nullptr) {
        LOG(ERROR) << "Module library is invalid or outdated: required interface function not found!";
        throw allpix::DynamicLibraryError(module_name);
    }
    // Convert to correct generator function
    auto module_generator = reinterpret_cast<Module* (*)(Configuration, Messenger*, GeometryManager*)>(generator); // NOLINT

    // Set the log section header
    std::string old_section_name = Log::getSection();
    std::string section_name = "C:";
    section_name += identifier.getUniqueName();
    Log::setSection(section_name);
    // set module specific log settings
    auto old_settings = set_module_before(identifier.getUniqueName(), config);
    // Build module
    Module* module = module_generator(config, messenger, geo_manager);
    // Reset log
    Log::setSection(old_section_name);
    set_module_after(old_settings);

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
                                                                                         const Configuration& config,
                                                                                         Messenger* messenger,
                                                                                         GeometryManager* geo_manager) {
    std::string module_name = config.getName();
    LOG(DEBUG) << "Creating instantions for detector module " << module_name;

    // Open the library and get the module generator function
    void* generator = dlsym(library, ALLPIX_GENERATOR_FUNCTION);
    // If the generator function was not found, throw an error
    if(generator == nullptr) {
        LOG(ERROR) << "Module library is invalid or outdated: required interface function not found!";
        throw allpix::DynamicLibraryError(module_name);
    }
    // Convert to correct generator function
    auto module_generator =
        reinterpret_cast<Module* (*)(Configuration, Messenger*, std::shared_ptr<Detector>)>(generator); // NOLINT

    // FIXME: Handle empty type and name arrays (or disallow them)
    std::vector<std::pair<std::shared_ptr<Detector>, ModuleIdentifier>> instantiations;

    // Create all names first with highest priority
    std::set<std::string> module_names;
    if(config.has("name")) {
        std::vector<std::string> names = config.getArray<std::string>("name");
        for(auto& name : names) {
            auto det = geo_manager->getDetector(name);
            instantiations.emplace_back(det, ModuleIdentifier(module_name, det->getName(), 0));

            // Save the name (to not instantiate it again later)
            module_names.insert(name);
        }
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

                instantiations.emplace_back(det, ModuleIdentifier(module_name, det->getName(), 1));
            }
        }
    }

    // Create for all detectors if no name / type provided
    if(!config.has("type") && !config.has("name")) {
        auto detectors = geo_manager->getDetectors();

        for(auto& det : detectors) {
            instantiations.emplace_back(det, ModuleIdentifier(module_name, det->getName(), 2));
        }
    }

    // Construct instantiations from the list of requests
    std::vector<std::pair<ModuleIdentifier, Module*>> module_list;
    for(auto& instance : instantiations) {
        // Set the log section header
        std::string old_section_name = Log::getSection();
        std::string section_name = "C:";
        section_name += instance.second.getUniqueName();
        Log::setSection(section_name);
        // Set module specific log settings
        auto old_settings = set_module_before(instance.second.getUniqueName(), config);
        // Build module
        Module* module = module_generator(config, messenger, instance.first);
        // Reset logging
        Log::setSection(old_section_name);
        set_module_after(old_settings);

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
