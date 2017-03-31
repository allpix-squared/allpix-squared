/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_ALLPIX_H
#define ALLPIX_ALLPIX_H

#include <memory>
#include <string>

#include "config/ConfigManager.hpp"
#include "geometry/GeometryManager.hpp"
#include "messenger/Messenger.hpp"
#include "module/ModuleManager.hpp"

namespace allpix {

    class AllPix {
    public:
        // Constructor and destructor
        AllPix(std::string name, std::unique_ptr<ModuleManager> mod_mgr);

        // Initialize to valid state
        void init();

        // Start a single run of every module
        void run();

        // Finalize run, check if everything has finished
        void finalize();

    private:
        // Set the default unit conventions in AllPix
        void add_units();

        // Managers
        std::unique_ptr<ConfigManager> conf_mgr_;
        std::unique_ptr<ModuleManager> mod_mgr_;
        std::unique_ptr<GeometryManager> geo_mgr_;
        std::unique_ptr<Messenger> msg_;
    };
} // namespace allpix

#endif /* ALLPIX_ALLPIX_H */
