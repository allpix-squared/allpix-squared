/**
 * @file
 * @brief Loading and execution of all modules
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MODULE_MANAGER_H
#define ALLPIX_MODULE_MANAGER_H

#include <atomic>
#include <list>
#include <map>
#include <memory>
#include <queue>

#include <TDirectory.h>
#include <TFile.h>
#include <TH1D.h>

#include "Module.hpp"
#include "ThreadPool.hpp"
#include "core/config/Configuration.hpp"
#include "core/utils/log.h"
#include "tools/ROOT.h"

namespace allpix {

    using ModuleList = std::list<std::shared_ptr<Module>>;

    class ConfigManager;
    class Messenger;
    class GeometryManager;

    /**
     * @ingroup Managers
     * @brief Manager responsible for dynamically loading all modules and running their event sequence
     *
     * Responsible for the following tasks:
     * - Loading the dynamic libraries
     * - Instantiating the modules from the libraries
     * - Initializng the modules
     * - Running the modules for every event
     * - Finalizing the modules
     */
    class ModuleManager {
        friend class Event;

    public:
        /**
         * @brief Construct manager
         */
        ModuleManager();
        /**
         * @brief Use default destructor
         */
        ~ModuleManager() = default;

        /// @{
        /**
         * @brief Copying the manager is not allowed
         */
        ModuleManager(const ModuleManager&) = delete;
        ModuleManager& operator=(const ModuleManager&) = delete;
        /// @}

        /// @{
        /**
         * @brief Moving the manager is not allowed
         */
        ModuleManager(ModuleManager&&) = delete;
        ModuleManager& operator=(ModuleManager&&) = delete;
        /// @}

        /**
         * @brief Dynamically load all the modules
         * @param messenger Pointer to the messenger
         * @param conf_manager Pointer to the configuration manager
         * @param geo_manager Pointer to the manager holding the geometry
         */
        void load(Messenger* messenger, ConfigManager* conf_manager, GeometryManager* geo_manager);

        /**
         * @brief Initialize all modules before the event sequence
         * @warning Should be called after the \ref ModuleManager::load "load function"
         */
        void initialize();

        /**
         * @brief Run all modules for the number of events
         * @param seeder Reference to the seeder
         * @warning Should be called after the \ref ModuleManager::initialize "init function"
         */
        void run(RandomNumberGenerator& seeder);

        /**
         * @brief Finalize all modules after the event sequence
         * @warning Should be called after the \ref ModuleManager::initialize "run function"
         */
        void finalize();

        /**
         * @brief Terminates as soon as the current event is finished
         * @note This method is safe to call from any signal handler
         */
        void terminate();

    private:
        /**
         * @brief Create unique modules
         * @param library Void pointer to the loaded library
         * @param config Configuration of the module
         * @param messenger Pointer to the messenger
         * @param geo_manager Pointer to the geometry manager
         * @return An unique module together with its identifier
         */
        std::pair<ModuleIdentifier, Module*> create_unique_modules(void*, Configuration&, Messenger*, GeometryManager*);

        /**
         * @brief Create detector modules
         * @param library Void pointer to the loaded library
         * @param config Configuration of the module
         * @param messenger Pointer to the messenger
         * @param geo_manager Pointer to the geometry manager
         * @return A list of all created detector modules and their identifiers
         */
        std::vector<std::pair<ModuleIdentifier, Module*>>
        create_detector_modules(void*, Configuration&, Messenger*, GeometryManager*);

        /**
         * @brief Set module specific log setting before running init/run/finalize
         * @param mod_name Unique identifier of the module
         * @param config   Module configuration
         * @param prefix   Possible section name prefix
         * @param event    Event number, defaults to 0 for not displaying it
         */
        static std::tuple<LogLevel, LogFormat, std::string, uint64_t> set_module_before(const std::string& mod_name,
                                                                                        const Configuration& config,
                                                                                        const std::string& prefix = "",
                                                                                        const uint64_t event = 0);

        /**
         * @brief Reset global log setting after running init/run/finalize
         * @param prev Set of previous settings generated by \ref set_module_before
         */
        static void set_module_after(std::tuple<LogLevel, LogFormat, std::string, uint64_t> prev);

        using IdentifierToModuleMap = std::map<ModuleIdentifier, ModuleList::iterator>;

        ModuleList modules_;
        IdentifierToModuleMap id_to_module_;

        ConfigManager* conf_manager_{};

        std::unique_ptr<TFile> modules_file_;

        // Duration in ns
        std::map<Module*, std::atomic_int64_t> module_execution_time_;
        std::map<Module*, Histogram<TH1D>> module_event_time_;
        Histogram<TH1D> event_time_;
        Histogram<TH1D> buffer_fill_level_;

        // Durations in ns
        uint64_t initialize_time_{}, run_time_{}, finalize_time_{};

        std::map<std::string, void*> loaded_libraries_;

        std::atomic<bool> terminate_;

        Messenger* messenger_{};

        // The thread pool used in the run method
        std::unique_ptr<ThreadPool> thread_pool_{nullptr};

        // User defined multithreading flags and parameters from configuration
        bool multithreading_flag_{false};
        unsigned int number_of_threads_{0};
        size_t max_buffer_size_{1};

        // Possibility of running loaded modules in parallel
        bool can_parallelize_{true};
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_MANAGER_H */
