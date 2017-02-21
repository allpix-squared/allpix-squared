/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "StaticModuleManager.hpp"

#include <memory>
#include <vector>
#include <utility>
#include <string>

#include "Module.hpp"
#include "../config/Configuration.hpp"
#include "../AllPix.hpp"
#include "../utils/exceptions.h"
#include "../utils/log.h"

using namespace allpix;

StaticModuleManager::StaticModuleManager(GeneratorFunction func): generator_func_(func) {
    if (generator_func_ == nullptr) throw allpix::Exception("Generator function should not be zero");
}

void StaticModuleManager::load(AllPix *allpix) {
    // get our copy to AllPix
    apx_ = allpix;

    // get configurations
    std::vector<Configuration> configs = apx_->getConfigManager()->getConfigurations();
    
    // NOTE: could add all config parameters from the empty to all configs (if it does not yet exist)    
    for (auto &conf : configs) {
        // ignore the empty config
        if (conf.getName().empty()) continue;
        
        // instantiate an instance for each name
        std::unique_ptr<ModuleFactory> factory = get_factory(conf.getName());
        factory->setAllPix(apx_);
        factory->setConfiguration(conf);
        std::vector<std::unique_ptr<Module> > mod_list = factory->create();
        
        for (auto &mod : mod_list) {
            // initialize the module 
            // FIXME: should move to the constructor
            mod->init(conf);
            
            // add the module to the run queue
            add_to_run_queue(mod.get());
        }
        
        // move the module pointers to the module list
        std::move(mod_list.begin(), mod_list.end(), std::back_inserter(modules_));  // ##
        mod_list.clear();
        
        // FIXME: config should be passed earlier
        // modules_.back()->init(conf);
    }
}

// get the factory for instantiation the modules
std::unique_ptr<ModuleFactory> StaticModuleManager::get_factory(std::string name) {
    std::unique_ptr<ModuleFactory> mod = generator_func_(name);
    if (mod == nullptr) {
        throw InstantiationError(name);
    }
    return mod;
}
