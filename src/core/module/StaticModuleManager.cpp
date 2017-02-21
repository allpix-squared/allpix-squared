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
            mod->init(conf);
        }
        
        // move the module pointers to the module list
        std::move(mod_list.begin(), mod_list.end(), std::back_inserter(modules_));  // ##
        mod_list.clear();
        
        // FIXME: config should be passed earlier
        // modules_.back()->init(conf);
    }
}

void StaticModuleManager::addToRunQueue(Module *mod) {
    run_queue_.push(mod);
}

void StaticModuleManager::run() {
    // go through the run queue as long it is not empty
    while (!run_queue_.empty()) {
        Module *mod = run_queue_.front();
        run_queue_.pop();
        
        mod->run();
    }
}

void StaticModuleManager::finalize() {
    // finalize the modules
    for (auto &mod : modules_) {
        mod->finalize();
    }
}

std::unique_ptr<ModuleFactory> StaticModuleManager::get_factory(std::string name) {
    std::unique_ptr<ModuleFactory> mod = generator_func_(name);
    if (mod == nullptr) {
        throw InstantiationError(name);
    }
    return mod;
}

/*void StaticModuleManager::add_module(std::string name, const Configuration& conf){
    LOG(DEBUG) << "adding module instance of name " << name;
    
    std::unique_ptr<Module> mod = std::unique_ptr<Module>(generator_func_(name, apx_));
    if(mod == nullptr){
        throw allpix::Exception("Module cannot be instantiated");
    }
    mod->init(conf);
    modules_.push_back(std::move(mod));
}*/
