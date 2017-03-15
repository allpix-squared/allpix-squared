/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "StaticModuleManager.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Module.hpp"

#include "core/AllPix.hpp"
#include "core/config/Configuration.hpp"
#include "core/utils/exceptions.h"
#include "core/utils/log.h"

using namespace allpix;

StaticModuleManager::StaticModuleManager(GeneratorFunction func) : _instantiations_map(), generator_func_(std::move(func)) {
    if(generator_func_ == nullptr) {
        throw allpix::Exception("Generator function should not be zero");
    }
}

void StaticModuleManager::load(AllPix* allpix) {
    // get configurations
    std::vector<Configuration> configs = allpix->getConfigManager()->getConfigurations();

    // NOTE: could add all config parameters from the empty to all configs (if it does not yet exist)
    for(auto& conf : configs) {
        // ignore the empty config
        if(conf.getName().empty()) {
            continue;
        }

        // instantiate an instance for each name
        std::unique_ptr<ModuleFactory> factory = get_factory(conf.getName());
        factory->setAllPix(allpix);
        factory->setConfiguration(conf);
        std::vector<std::pair<ModuleIdentifier, std::unique_ptr<Module>>> mod_list = factory->create();

        for(auto&& id_mod : mod_list) {
            std::unique_ptr<Module> &mod = id_mod.second;
            ModuleIdentifier identifier = id_mod.first;
            
            // initialize the module
            // FIXME: should move to the constructor

            // mod->init(conf);

            // add the module to the run queue
            // add_to_run_queue(mod.get());

            auto             iter = id_to_module_.find(identifier);
            if(iter != id_to_module_.end()) {
                // unique name already exists, check if its needs to be replaced
                if(iter->first.getPriority() > identifier.getPriority()) {
                    // priority of new instance is higher, replace the instance
                    modules_.erase(iter->second);
                    module_to_id_.erase(iter->second->get());
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

        // FIXME: config should be passed earlier
        // modules_.back()->init(conf);
    }

    // initialize all all remaining modules and add them to the run queue

    for(auto& mod : modules_) {
        mod->init();

        add_to_run_queue(mod.get());
    }
}

// get the factory for instantiation the modules
std::unique_ptr<ModuleFactory> StaticModuleManager::get_factory(const std::string& name) {
    std::unique_ptr<ModuleFactory> mod = generator_func_(name);
    if(mod == nullptr) {
        throw InstantiationError(name);
    }
    return mod;
}
