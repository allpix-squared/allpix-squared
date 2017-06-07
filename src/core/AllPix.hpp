/** @file
 *  @brief Interface to the core framework
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

#include <atomic>
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
         * @brief Constructs AllPix and initialize all managers
         * @param config_file_name Path of the main configuration file
         */
        explicit AllPix(std::string config_file_name);

        /**
         * @brief Load modules from the main configuration and construct them
         * @warning Should be called after the \ref AllPix() "constructor"
         */
        void load();

        /**
         * @brief Initialize all modules (pre-run)
         * @warning Should be called after the \ref AllPix::load "load function"
         */
        void init();

        /**
         * @brief Run all modules for the number of events (run)
         * @warning Should be called after the \ref AllPix::init "init function"
         */
        void run();

        /**
         * @brief Finalize all modules (post-run)
         * @warning Should be called after the \ref AllPix::run "run function"
         */
        void finalize();

        /**
         * @brief Request termination as early as possible without changing the standard flow
         */
        void terminate();

    private:
        /**
         * @brief Sets the default unit conventions
         */
        void add_units();

        /**
         * @brief Set the default ROOT plot style
         */
        void set_style();

        // Indicate the framework should terminate
        std::atomic<bool> terminate_;
        std::atomic<bool> has_run_;

        // All managers in the framework
        std::unique_ptr<Messenger> msg_;
        std::unique_ptr<ModuleManager> mod_mgr_;
        std::unique_ptr<ConfigManager> conf_mgr_;
        std::unique_ptr<GeometryManager> geo_mgr_{};
    };
} // namespace allpix

#endif /* ALLPIX_ALLPIX_H */
