/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MODULE_MANAGER_H
#define ALLPIX_MODULE_MANAGER_H

#include <vector>
#include <queue>
#include <memory>

#include "Module.hpp"

namespace allpix {

    class AllPix;
    
    class ModuleManager{
    public:
        // Constructor and destructors
        ModuleManager() {}
        virtual ~ModuleManager() {}
        
        // Disallow copy
        ModuleManager(const ModuleManager&) = delete;
        ModuleManager &operator=(const ModuleManager&) = delete;
        
        // Load modules
        virtual void load(AllPix*) = 0;
        
        // Run modules (until no other module available)
        virtual void run();
        
        // Finalize and check if every module did what it should do
        virtual void finalize();
        
    protected:
        std::vector<std::unique_ptr<Module>> modules_;
        std::queue<Module*> run_queue_;
        
        // Add module to run queue
        void add_to_run_queue(Module *);
    };
}

#endif /* ALLPIX_MODULE_MANAGER_H */
