/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "StaticModuleManager.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../AllPix.hpp"
#include "../config/Configuration.hpp"
#include "../utils/exceptions.h"
#include "../utils/log.h"
#include "Module.hpp"

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
        std::vector<std::unique_ptr<Module>> mod_list = factory->create();

        for(auto&& mod : mod_list) {
            // initialize the module
            // FIXME: should move to the constructor

            // mod->init(conf);

            // add the module to the run queue
            // add_to_run_queue(mod.get());

            ModuleIdentifier identifier = mod->getIdentifier();
            auto             iter = identifiers_.find(identifier.getUniqueName());
            if(iter != identifiers_.end()) {
                // unique name already exists, check if its needs to be replaced
                if(iter->second.first.getPriority() > mod->getIdentifier().getPriority()) {
                    // priority of new instance is higher, replace the instance
                    modules_.erase(iter->second.second);
                    iter = identifiers_.erase(iter);
                } else {
                    if(iter->second.first.getPriority() == mod->getIdentifier().getPriority()) {
                        throw AmbiguousInstantiationError(conf.getName());
                    }
                    // priority is lower just ignore
                    continue;
                }
            }

            // insert the new module
            modules_.emplace_back(std::move(mod));
            std::pair<ModuleIdentifier, ModuleList::iterator> pair = std::make_pair(identifier, --modules_.end());
            identifiers_.emplace(identifier.getUniqueName(), pair);
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
