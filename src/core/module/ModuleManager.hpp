/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MODULE_MANAGER_H
#define ALLPIX_MODULE_MANAGER_H

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
        
        // Modify run queue
        // FIXME: right name?
        virtual void addToRunQueue(Module *) = 0; 
        
        // Load modules
        virtual void load(AllPix*) = 0;
        
        // Run modules (until no other module available)
        virtual void run() = 0;
        
        // Finalize and check if every module did what it should do
        virtual void finalize() = 0;
    };
}

#endif /* ALLPIX_MODULE_MANAGER_H */
