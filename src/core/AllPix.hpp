/** @file
 *  @brief Interface class to the core framework
 *  @copyright MIT License
 */

// GLOBAL DOXYGEN DEFINITIONS
/**
 * @defgroup Managers Managers
 * @brief The global set of managers used in framework
 */
// END DEFINITIONS

#ifndef ALLPIX_ALLPIX_H
#define ALLPIX_ALLPIX_H

#include <memory>
#include <string>

#include "config/ConfigManager.hpp"
#include "geometry/GeometryManager.hpp"
#include "messenger/Messenger.hpp"
#include "module/ModuleManager.hpp"

namespace allpix {
    /**
     * @brief Provides the link between the core framework and the executable.
     *
     * Supply the path location the main configuration which should be provided to the executable. Hereafter this class
     * should be used to load, initialize, run and finalize all the modules.
     */

    class AllPix {
    public:
        /**
         * @brief Constructs an object with the location of the main configuration
         * @param file_name Path of the main configuration file
         */
        explicit AllPix(std::string file_name);

        /**
         * @brief Load modules from the main configuration and construct them
         */
        void load();

        /**
         * @brief Initialize all modules (pre-run)
         */
        void init();

        /**
         * @brief Run all modules for the number of events (run)
         */
        void run();

        /**
         * @brief Finalize all modules (post-run)
         */
        void finalize();

    private:
        /**
         * @brief Sets the default unit conventions
         */
        void add_units();

        // All managers in the framework
        std::unique_ptr<ConfigManager> conf_mgr_;
        std::unique_ptr<ModuleManager> mod_mgr_;
        std::unique_ptr<GeometryManager> geo_mgr_;
        std::unique_ptr<Messenger> msg_;
    };
} // namespace allpix

#endif /* ALLPIX_ALLPIX_H */
