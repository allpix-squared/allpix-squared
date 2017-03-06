/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MODULE_MANAGER_H
#define ALLPIX_MODULE_MANAGER_H

#include <list>
#include <queue>
#include <memory>
#include <string>
#include <map>

#include "Module.hpp"

namespace allpix {

    class AllPix;
    
    class ModuleManager{
    public:
        // Constructor and destructors
        ModuleManager();
        virtual ~ModuleManager();
        
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
        using ModuleList = std::list<std::unique_ptr<Module>>;
        using ModuleIdentifierMap = std::map<std::string, std::pair<ModuleIdentifier, ModuleList::iterator> >;
        
        // Add module to run queue
        void add_to_run_queue(Module *);
        
        ModuleList modules_;
        ModuleIdentifierMap identifiers_;
        
        // FIXME: if we stick to the linear run we can also just run modules directly from list
        std::queue<Module*> run_queue_;
    };
}

#endif /* ALLPIX_MODULE_MANAGER_H */
