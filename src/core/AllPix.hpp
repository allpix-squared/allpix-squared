/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_ALLPIX_H
#define ALLPIX_ALLPIX_H

#include <map>
#include <vector>

#include "config/ConfigManager.hpp"
#include "geometry/GeometryManager.hpp"
#include "messenger/Messenger.hpp"
#include "module/ModuleManager.hpp"

namespace allpix {

    class AllPix {
    public:
        // Allpix running state
        enum State { Unitialized = 0, Initializing, Initialized, Running, Finished, Finalizing, Finalized };

        // Constructor and destructor
        // NOTE: AllPix object responsible for deletion of managers
        AllPix(std::unique_ptr<ConfigManager>   conf_mgr,
               std::unique_ptr<ModuleManager>   mod_mgr,
               std::unique_ptr<GeometryManager> geo_mgr);

        // Get managers
        // FIXME: pass shared pointers here?
        ConfigManager*   getConfigManager();
        GeometryManager* getGeometryManager();
        ModuleManager*   getModuleManager();
        Messenger*       getMessenger();

        // Get external managers
        // FIXME: is this a good place to implement this?
        template <typename T> std::shared_ptr<T> getExternalManager();
        template <typename T> void               setExternalManager(std::shared_ptr<T>);

        // Get state
        State getState() const;

        // Initialize to valid state
        void init();

        // Start a single run of every module
        void run();

        // Finalize run, check if everything has finished
        void finalize();

    private:
        std::unique_ptr<ConfigManager>   conf_mgr_;
        std::unique_ptr<ModuleManager>   mod_mgr_;
        std::unique_ptr<GeometryManager> geo_mgr_;
        std::unique_ptr<Messenger>       msg_;

        State state_;

        std::map<std::type_index, std::shared_ptr<void>> external_managers_;
    };

    template <typename T> std::shared_ptr<T> AllPix::getExternalManager() {
        // FIXME: crash here if the manager is not available ??
        return std::static_pointer_cast<T>(external_managers_[typeid(T)]);
    }
    template <typename T> void AllPix::setExternalManager(std::shared_ptr<T> model) {
        external_managers_[typeid(T)] = std::static_pointer_cast<void>(model);
    }
}

#endif /* ALLPIX_ALLPIX_H */
