/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_STATIC_MODULE_MANAGER_H
#define ALLPIX_STATIC_MODULE_MANAGER_H

#include <vector>
#include <queue>
#include <memory>

#include "Module.hpp"
#include "ModuleManager.hpp"

namespace allpix{

    class StaticModuleManager : public ModuleManager{
    public:
        // Constructor and destructors
        StaticModuleManager(Module* (*func)(std::string));
        ~StaticModuleManager() {}
        
        // Load modules
        void load(AllPix *);
        
        // Add to run queue
        void addToRunQueue(Module *);
        
        // Run modules (until no other module available)
        void run();
        
        // Finalize and check if every module did what it should do
        void finalize();
    private:
        void add_module(std::string name);
        
        std::vector<std::unique_ptr<Module>> modules_;
        std::queue<Module*> run_queue_;
        
        Module* (*generator_func_)(std::string);
        
        AllPix *allpix_;
    };
}

#endif // ALLPIX_STATIC_MODULE_MANAGER_H
