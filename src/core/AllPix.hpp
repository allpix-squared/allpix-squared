/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_ALLPIX_H
#define ALLPIX_ALLPIX_H

#include <vector>
#include <map>

#include "config/ConfigManager.hpp"
#include "geometry/GeometryManager.hpp"
#include "module/ModuleManager.hpp"
#include "messenger/Messenger.hpp"

namespace allpix{
    
    class AllPix {
    public:
        // Allpix running state
        enum State{
            Unitialized = 0,
            Initialized,
            Running,
            Finalized
        };
        
        // Constructor and destructor
        // NOTE: AllPix object responsible for deletion of managers
        AllPix(); // FIXME: temporary
        AllPix(ConfigManager *conf_mgr, ModuleManager *mod_mgr, GeometryManager *geo_mgr, Messenger *msg);
        ~AllPix();
        
        // Get managers
        ConfigManager *getConfigManager();
        GeometryManager *getGeometryManager();
        ModuleManager *getModuleManager();
        Messenger *getMessenger();
        
        // Get state
        State getState() const;
        
        // Initialize to valid state
        void init();
        
        // Start a single run of every module
        void run();
        
        // Finalize run, check if everything has finished
        void finalize();
    private:
        ConfigManager *conf_mgr_;
        ModuleManager *mod_mgr_;
        GeometryManager *geo_mgr_;
        Messenger *msg_;
        
        State state_;
    };
}

#endif // ALLPIX_ALLPIX_H
