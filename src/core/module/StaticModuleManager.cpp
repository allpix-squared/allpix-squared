/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "StaticModuleManager.hpp"

#include <memory>

#include "Module.hpp"
#include "../config/Configuration.hpp"
#include "../AllPix.hpp"
#include "../utils/exceptions.h"

using namespace allpix;

StaticModuleManager::StaticModuleManager(GeneratorFunction func): generator_func_(func) {
    if(generator_func_ == nullptr) throw allpix::exception("Generator function should not be zero");
}

void StaticModuleManager::load(AllPix *allpix){
    allpix_ = allpix;
    
    Configuration conf("test");
    conf.set("name", "a_random_name");
    
    // instantiate a module by name
    add_module("test1", conf);
    add_module("test2", conf);
}

void StaticModuleManager::addToRunQueue(Module *mod){
    run_queue_.push(mod);
}

void StaticModuleManager::run(){
    //go through the run queue as long it is not empty
    while(!run_queue_.empty()){
        Module *mod = run_queue_.front();
        run_queue_.pop();
        
        mod->run();
    }
}

void StaticModuleManager::finalize(){
    //finalize the modules
    for(auto &mod : modules_){
        mod->finalize();
    }
}

void StaticModuleManager::add_module(std::string name, const Configuration& conf){
    std::unique_ptr<Module> mod = std::unique_ptr<Module>(generator_func_(name, allpix_));
    if(mod == nullptr){
        throw allpix::exception("Module cannot be instantiated");
    }
    mod->init(conf);
    modules_.push_back(std::move(mod));
}
