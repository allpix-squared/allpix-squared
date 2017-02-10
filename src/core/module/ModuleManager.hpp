/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MODULE_MANAGER_H
#define ALLPIX_MODULE_MANAGER_H

#include <vector>

namespace allpix{

    class ModuleManager{
    public:
        // Constructor and destructors
        ModuleManager() {}
        virtual ~ModuleManager() {}
        
        // Disallow copy
        ModuleManager(const ModuleManager&) = delete;
        ModuleManager &operator=(const ModuleManager&) = delete;
        
        // Load modules
        virtual void load() = 0;
        
        // Run modules (until no other module available)
        virtual void run() = 0;
        
        // Finalize and check if every module did what it should do
        virtual void finalize() = 0;
    };
}

#endif // ALLPIX_MODULE_MANAGER_H
