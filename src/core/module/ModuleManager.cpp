/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 *  @author Daniel Hynds <daniel.hynds@cern.ch>
 */

#include <dlfcn.h>

#include "ModuleFactory.hpp"
#include "ModuleManager.hpp"
#include "core/config/Configuration.hpp"
#include "core/utils/log.h"

using namespace allpix;

// Constructor and destructor
ModuleManager::ModuleManager() : modules_(), id_to_module_(), module_to_id_(), global_config_() {}
ModuleManager::~ModuleManager() = default;

// Initialize all modules
void ModuleManager::init() {
    // initialize all modules
    for(auto& mod : modules_) {
        // set init module section header
        std::string old_section_name = Log::getSection();
        std::string section_name = "I:";
        section_name += module_to_id_.at(mod.get()).getName();
        Log::setSection(section_name);
        // init module
        mod->init();
        // reset header
        Log::setSection(old_section_name);
    }
}

// Run all the modules in the queue
void ModuleManager::run() {
    // loop over the number of events
    auto number_of_events = global_config_.get<unsigned int>("number_of_events", 1u);
    for(unsigned int i = 0; i < number_of_events; ++i) {
        // go through each module run method every event
        LOG(DEBUG) << "Running event " << (i + 1) << " of " << number_of_events;
        for(auto& mod : modules_) {
            // set run module section header
            std::string old_section_name = Log::getSection();
            std::string section_name = "R:";
            section_name += module_to_id_.at(mod.get()).getName();
            Log::setSection(section_name);
            // run module
            mod->run();
            // reset header
            Log::setSection(old_section_name);
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
        section_name += module_to_id_.at(mod.get()).getName();
        Log::setSection(section_name);
        // finalize module
        mod->finalize();
        // reset header
        Log::setSection(old_section_name);
    }
}

// Function that loads the modules specified in the configuration file. Each module is contained within
// its own library, which is first loaded before being passed to the module factory
void ModuleManager::load(Messenger* messenger, ConfigManager* conf_manager, GeometryManager* geo_manager) {

    // Get configurations
    std::vector<Configuration> configs = conf_manager->getConfigurations();

    // NOTE: could add all config parameters from the empty to all configs (if it does not yet exist)
    std::unique_ptr<ModuleFactory> factory = std::make_unique<ModuleFactory>();
    for(auto& conf : configs) {
        // ignore the empty config
        if(conf.getName().empty()) {
            continue;
        }

        // Load library for each module. Libraries are named (by convention + CMAKE) libModuleName.suffix
        std::string libName = std::string("libAllpixModule").append(conf.getName()).append(".dylib");
        LOG(INFO) << "Loading library " << libName << std::endl;
        std::string libPath = std::string(std::getenv("ALLPIX_DIR")) + "/lib/";
        //        std::string libName = std::string("lib").append(conf.getName()).append(SHARED_LIBRARY_SUFFIX);
        std::string fullLibPath = libPath + libName;

        // If library is not loaded then load it
        if(loadedLibraries_.count(libName) == 0) {
            void* lib = nullptr;
            lib = dlopen(fullLibPath.c_str(), RTLD_NOW);
            // If library did not load then throw exception
            if(!lib) {
                LOG(ERROR) << "Library " << libName << " not loaded" << std::endl;
                LOG(ERROR) << " - Did you set the ALLPIX_DIR environmental variable? Library search location: " << libPath
                           << std::endl;
                LOG(ERROR) << " - Did you compile the library? " << std::endl;
                LOG(ERROR) << " - Did you spell the library name correctly? " << std::endl;
                throw allpix::DynamicLibraryError(conf.getName());
            }
            // Remember that this library was loaded
            loadedLibraries_[libName] = lib;
        }

        // Pass the library to the module factory
        factory->setMessenger(messenger);
        factory->setGeometryManager(geo_manager);
        factory->setConfiguration(conf);
        std::vector<std::pair<ModuleIdentifier, Module*>> mod_list =
            factory->createModules(conf.getName(), loadedLibraries_[libName]);

        // Decide which order to place modules in
        for(auto& id_mod : mod_list) {
            Module* mod = id_mod.second;
            ModuleIdentifier identifier = id_mod.first;

            // Need to add delete statements here since move from unique pointer?
            auto iter = id_to_module_.find(identifier);
            if(iter != id_to_module_.end()) {
                // unique name already exists, check if its needs to be replaced
                if(iter->first.getPriority() > identifier.getPriority()) {
                    // priority of new instance is higher, replace the instance
                    module_to_id_.erase(iter->second->get());
                    modules_.erase(iter->second);
                    id_to_module_.erase(iter->first);
                } else {
                    if(iter->first.getPriority() == identifier.getPriority()) {
                        throw AmbiguousInstantiationError(conf.getName());
                    }
                    // priority is lower just ignore
                    continue;
                }
            }

            // insert the new module
            modules_.emplace_back(std::move(mod));
            id_to_module_[identifier] = --modules_.end();
            module_to_id_.emplace(modules_.back().get(), identifier);
        }
        mod_list.clear();
    }
}
