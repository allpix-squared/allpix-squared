/**
 * @file
 * @brief Interface to the core framework
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

/**
 * @defgroup Managers Managers
 * @brief The global set of managers used in framework
 */

#ifndef ALLPIX_ALLPIX_H
#define ALLPIX_ALLPIX_H

#include <atomic>
#include <fstream>
#include <memory>
#include <stop_token>
#include <string>
#include <thread>

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

    class Allpix {
    public:
        /**
         * @brief Constructs Allpix and initialize all managers
         * @param config_file_name Path of the main configuration file
         * @param module_options List of extra configuration options for modules
         * @param detector_options List of extra configuration options for the geometry setup
         */
        explicit Allpix(std::filesystem::path config_file_name,
                        const std::vector<std::string>& module_options = std::vector<std::string>(),
                        const std::vector<std::string>& detector_options = std::vector<std::string>());

        /**
         * @brief Load modules from the main configuration and construct them
         * @warning Should be called after the \ref Allpix::Allpix() "constructor"
         */
        void load();

        /**
         * @brief Initialize all modules (pre-run)
         * @warning Should be called after the \ref Allpix::load "load function"
         */
        void initialize();

        /**
         * @brief Start the simulation run
         * @details Starts the thread with the event loop
         */
        void start();

        /**
         * @brief Check if the run thread has thrown an exception
         * @throw Exception thrown by run thread, if any
         */
        void checkException();

        /**
         * @brief Wait for the event loop to finish
         */
        void wait();

        /**
         * @brief Request interruption of event loop
         */
        void interrupt();

        /**
         * @brief Finalize all modules (post-run)
         * @warning Should be called after the \ref Allpix::run "run function"
         */
        void finalize();

    private:
        /**
         * @brief Run all modules for the number of events (run)
         * @param stop_token Stop token to interrupt the run
         * @warning Should be called after the \ref Allpix::initialize "init function"
         */
        void run(const std::stop_token& stop_token);

        /**
         * @brief Helper function to load all configurations into the ConfigManager
         * @details This method abstracts file parsing of the main configuration file as well as the geometry file and loads
         *          module configuration and detectors and applied the options provided via the command line
         *
         * @param config_file_name File name of the main configuration file
         * @param module_options Command-line options for modules
         * @param detector_options Command-line options for the geometry
         */
        void load_configuration(std::filesystem::path config_file_name,
                                const std::vector<std::string>& module_options,
                                const std::vector<std::string>& detector_options);

        /**
         * @brief Helper function to load information into the GeometryManager
         */
        void load_geometry();

        /**
         * @brief Returns the list of standard paths where models should be searched in
         * @return List of absolute paths to file or directories that contain models
         *
         * The default list of models to search for are in the following order
         * - The list of paths provided in the main configuration as model_paths
         * - The build variable ALLPIX_MODEL_DIR pointing to the installation directory of the framework models
         * - The directories in XDG_DATA_DIRS attached by ALLPIX_PROJECT_NAME or /usr/share/:/usr/local/share if not defined
         */
        std::vector<std::filesystem::path> get_model_paths(const std::filesystem::path& config_file_path);

        void read_model_file(const std::filesystem::path& path);

        /**
         * @brief Set the default ROOT plot style
         */
        static void set_style();

        // Indicate the framework should terminate
        std::atomic<bool> has_run_;
        std::jthread run_thread_;
        std::stop_source stop_source_;

        // Exception pointer
        std::exception_ptr exception_ptr_{nullptr};

        // Log file if specified
        std::ofstream log_file_;
        LogLevel log_level_{LogLevel::WARNING};

        // All managers in the framework
        std::unique_ptr<Messenger> msg_;
        std::unique_ptr<ConfigManager> conf_mgr_;
        std::unique_ptr<GeometryManager> geo_mgr_;
        std::unique_ptr<ModuleManager> mod_mgr_;
        std::unique_ptr<HistogramManager> histo_mgr_;

        // Random generators
        RandomNumberGenerator seeder_modules_;
        RandomNumberGenerator seeder_core_;
    };
} // namespace allpix

#endif /* ALLPIX_ALLPIX_H */
