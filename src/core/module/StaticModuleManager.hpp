/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_STATIC_MODULE_MANAGER_H
#define ALLPIX_STATIC_MODULE_MANAGER_H

#include <vector>
#include <queue>
#include <memory>
#include <functional>

#include "Module.hpp"
#include "ModuleManager.hpp"

namespace allpix{

    class AllPix;
    
    class StaticModuleManager : public ModuleManager{
    public:
        using GeneratorFunction = std::function<Module*(std::string, AllPix*)>;
        
        // Constructor and destructors
        StaticModuleManager(GeneratorFunction);
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
        void add_module(std::string name, const Configuration&);
        
        std::vector<std::unique_ptr<Module>> modules_;
        std::queue<Module*> run_queue_;
        
        GeneratorFunction generator_func_;
        
        AllPix *allpix_;
    };
}

#endif // ALLPIX_STATIC_MODULE_MANAGER_H
