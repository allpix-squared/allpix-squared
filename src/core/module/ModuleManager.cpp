/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 *  @author Daniel Hynds <daniel.hynds@cern.ch>
 */

#include <dlfcn.h>

#include <cstring>
#include <fstream>
#include <set>
#include <string>

#include "ModuleManager.hpp"
#include "core/config/ConfigManager.hpp"
#include "core/config/Configuration.hpp"
#include "core/config/exceptions.h"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/utils/log.h"

// NOTE: should be the same as in dynamic_module_impl.cpp
#define ALLPIX_MODULE_PREFIX "libAllpixModule"
#define ALLPIX_GENERATOR_FUNCTION "allpix_module_generator"
#define ALLPIX_UNIQUE_FUNCTION "allpix_module_is_unique"

using namespace allpix;

// Constructor and destructor
ModuleManager::ModuleManager() : modules_(), id_to_module_(), global_config_(), loaded_libraries_() {}
ModuleManager::~ModuleManager() = default;

// Function that loads the modules specified in the configuration file. Each module is contained within
// its own library, which is first loaded before being used to create the modules
void ModuleManager::load(Messenger* messenger, ConfigManager* conf_manager, GeometryManager* geo_manager) {
    // Get configurations
    std::vector<Configuration> configs = conf_manager->getConfigurations();
    global_config_ = conf_manager->getGlobalConfiguration();

    // NOTE: could add all config parameters from the empty to all configs (if it does not yet exist)
    for(auto& conf : configs) {
        // ignore the empty config
        if(conf.getName().empty()) {
            continue;
        }

        // Load library for each module. Libraries are named (by convention + CMAKE) libAllpixModule Name.suffix
        std::string lib_name = std::string(ALLPIX_MODULE_PREFIX).append(conf.getName()).append(SHARED_LIBRARY_SUFFIX);
        LOG(INFO) << "Loading library " << lib_name;

        // If library is not loaded then try to load it
        void* lib = nullptr;
        dlerror(); // reset error stream
        bool load_error = false;
        if(loaded_libraries_.count(lib_name) == 0) {
            // Go through the config file directories
            if(global_config_.has("library_directories")) {
                std::vector<std::string> lib_paths = global_config_.getPathArray("library_directories", true);
                for(auto& lib_path : lib_paths) {
                    std::string full_lib_path = lib_path + "/" + lib_name;

                    // check if file exists and try to load if it exists
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

            throw allpix::DynamicLibraryError(conf.getName());
        }
        // Remember that this library was loaded
        loaded_libraries_[lib_name] = lib;

        // Check if this module is produced once, or once per detector
        bool unique = true;
        void* uniqueFunction = dlsym(loaded_libraries_[lib_name], ALLPIX_UNIQUE_FUNCTION);

        // If the unique function was not found, throw an error
        if(uniqueFunction == nullptr) {
            LOG(ERROR) << "Module library is invalid or outdated: required interface function not found!";
            throw allpix::DynamicLibraryError(conf.getName());
        } else {
            unique = reinterpret_cast<bool (*)()>(uniqueFunction)(); // NOLINT
        }

        // Create the modules from the library
        std::vector<std::pair<ModuleIdentifier, Module*>> mod_list;
        if(unique) {
            mod_list.emplace_back(create_unique_modules(loaded_libraries_[lib_name], conf, messenger, geo_manager));
        } else {
            mod_list = create_detector_modules(loaded_libraries_[lib_name], conf, messenger, geo_manager);
        }

        // Decide which order to place modules in
        for(auto& id_mod : mod_list) {
            // NOTE: this convert the module to an unique pointer
            // FIXME: check that this always works and we can better do this earlier
            std::unique_ptr<Module> mod(id_mod.second);
            ModuleIdentifier identifier = id_mod.first;

            // FIXME: need to add delete statements here since move from unique pointer?
            auto iter = id_to_module_.find(identifier);
            if(iter != id_to_module_.end()) {
                // unique name already exists, check if its needs to be replaced
                if(identifier.getPriority() < iter->first.getPriority()) {
                    // priority of new instance is higher, replace the instance
                    iter->second = modules_.erase(iter->second);
                    iter = id_to_module_.erase(iter);
                } else {
                    if(identifier.getPriority() == iter->first.getPriority()) {
                        throw AmbiguousInstantiationError(conf.getName());
                    }
                    // priority is lower just ignore
                    continue;
                }
            }

            // set the configuration for this module for later use
            mod->set_configuration(conf);

            // insert the new module
            modules_.emplace_back(std::move(mod));
            id_to_module_[identifier] = --modules_.end();

            // save the identifier in the module
            modules_.back()->set_identifier(identifier);
        }
        mod_list.clear();
    }
}

// Helper functions to set the module specific log settings if necessary
static std::pair<LogLevel, LogFormat> set_module_log(const Configuration& config) {
    // set new level if necessary
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

    // set new format if necessary
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
    return std::make_pair(prev_level, prev_format);
}
static void reset_module_log(std::pair<LogLevel, LogFormat> prev) {
    LogLevel cur_level = Log::getReportingLevel();
    if(cur_level != prev.first) {
        Log::setReportingLevel(prev.first);
        LOG(DEBUG) << "Reset log level to global level of " << Log::getStringFromLevel(prev.first);
    }

    LogFormat cur_format = Log::getFormat();
    if(cur_format != prev.second) {
        Log::setFormat(prev.second);
        LOG(DEBUG) << "Reset log format to global level of " << Log::getStringFromFormat(prev.second);
    }
}

// Initialize all modules
void ModuleManager::init() {
    // initialize all modules
    for(auto& mod : modules_) {
        // set init module section header
        std::string old_section_name = Log::getSection();
        std::string section_name = "I:";
        section_name += mod->get_identifier().getUniqueName();
        Log::setSection(section_name);
        // set module specific log settings
        auto old_settings = set_module_log(mod->get_configuration());
        // init module
        mod->init();
        // resetting bound messages
        LOG(DEBUG) << "Clearing bound messages";
        mod->reset_delegates();
        // reset log
        Log::setSection(old_section_name);
        reset_module_log(old_settings);
    }
}

// Run all the modules in the queue
void ModuleManager::run() {
    // loop over the number of events
    auto number_of_events = global_config_.get<unsigned int>("number_of_events", 1u);
    for(unsigned int i = 0; i < number_of_events; ++i) {
        // go through each module run method every event
        LOG(INFO) << "Running event " << (i + 1) << " of " << number_of_events;
        for(auto& mod : modules_) {
            // check if module is clear to run
            if(!mod->check_delegates()) {
                LOG(DEBUG) << "Not all required messages are received for " << mod->get_identifier().getUniqueName()
                           << "... skipping!";
                continue;
            }

            // set run module section header
            std::string old_section_name = Log::getSection();
            std::string section_name = "R:";
            section_name += mod->get_identifier().getUniqueName();
            Log::setSection(section_name);
            // set module specific log settings
            auto old_settings = set_module_log(mod->get_configuration());
            // run module
            mod->run();
            // resetting bound messages
            LOG(DEBUG) << "Resetting bound messages";
            mod->reset_delegates();
            // reset log
            Log::setSection(old_section_name);
            reset_module_log(old_settings);
        }
    }
}

// Finalize the modules
void ModuleManager::finalize() {
    // finalize the modules
    for(auto& mod : modules_) {
        // set finalize module section header
        std::string old_section_name = Log::getSection();
        std::string section_name = "F:";
        section_name += mod->get_identifier().getUniqueName();
        Log::setSection(section_name);
        // set module specific log settings
        auto old_settings = set_module_log(mod->get_configuration());
        // finalize module
        mod->finalize();
        // reset log
        Log::setSection(old_section_name);
        reset_module_log(old_settings);
    }
}

// Function to create modules from the dynamic library passed from the Module Manager
std::pair<ModuleIdentifier, Module*> ModuleManager::create_unique_modules(void* library,
                                                                          const Configuration& conf,
                                                                          Messenger* messenger,
                                                                          GeometryManager* geo_manager) {
    // Make the vector to return
    std::string module_name = conf.getName();

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
    section_name += module_name;
    Log::setSection(section_name);
    // set module specific log settings
    auto old_settings = set_module_log(conf);
    // Build module
    Module* module = module_generator(conf, messenger, geo_manager);
    // Reset log
    Log::setSection(old_section_name);
    reset_module_log(old_settings);

    // Store the module and return it to the Module Manager
    return std::make_pair(identifier, module);
}

// Function to create modules per detector from the dynamic library passed from the Module Manager
std::vector<std::pair<ModuleIdentifier, Module*>> ModuleManager::create_detector_modules(void* library,
                                                                                         const Configuration& conf,
                                                                                         Messenger* messenger,
                                                                                         GeometryManager* geo_manager) {
    std::string module_name = conf.getName();
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

    // FIXME: check empty config arrays
    std::vector<std::pair<std::shared_ptr<Detector>, ModuleIdentifier>> instantiations;

    // create all names first with highest priority
    std::set<std::string> module_names;
    if(conf.has("name")) {
        std::vector<std::string> names = conf.getArray<std::string>("name");
        for(auto& name : names) {
            // get detectors by name
            auto det = geo_manager->getDetector(name);
            instantiations.emplace_back(det, ModuleIdentifier(module_name, det->getName(), 0));

            // save the name (to not override it later)
            module_names.insert(name);
        }
    }

    // then create all types that are not yet name instantiated
    if(conf.has("type")) {
        std::vector<std::string> types = conf.getArray<std::string>("type");
        for(auto& type : types) {
            auto detectors = geo_manager->getDetectorsByType(type);

            for(auto& det : detectors) {
                // skip all that were already added by name
                if(module_names.find(det->getName()) != module_names.end()) {
                    continue;
                }

                instantiations.emplace_back(det, ModuleIdentifier(module_name, det->getName(), 1));
            }
        }
    }

    // create for all detectors if no name / type provided
    if(!conf.has("type") && !conf.has("name")) {
        auto detectors = geo_manager->getDetectors();

        for(auto& det : detectors) {
            instantiations.emplace_back(det, ModuleIdentifier(module_name, det->getName(), 2));
        }
    }

    // instantiate the list of requested instantiations
    std::vector<std::pair<ModuleIdentifier, Module*>> module_list;
    for(auto& instance : instantiations) {
        // Set the log section header
        std::string old_section_name = Log::getSection();
        std::string section_name = "C:";
        section_name += module_name;
        section_name += ":";
        section_name += instance.first->getName();
        Log::setSection(section_name);
        // set module specific log settings
        auto old_settings = set_module_log(conf);
        // Build module
        Module* module = module_generator(conf, messenger, instance.first);
        // Reset log
        Log::setSection(old_section_name);
        reset_module_log(old_settings);

        // check if the module called the correct base class constructor
        check_module_detector(module_name, module, instance.first.get());

        // store the module
        module_list.emplace_back(instance.second, module);
    }

    return module_list;
}

// Check if a detector module correctly passes the detector to the base constructor
void ModuleManager::check_module_detector(const std::string& module_name, Module* module, const Detector* detector) {
    if(module->getDetector().get() != detector) {
        throw InvalidModuleStateException(
            "Module " + module_name +
            " does not call the correct base Module constructor: the provided detector should be forwarded");
    }
}
