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
#include <stop_token>

#include <TDirectory.h>
#include <TFile.h>
#include <TH1D.h>

#include "Module.hpp"
#include "ThreadPool.hpp"
#include "core/config/Configuration.hpp"
#include "core/histograms/HistogramManager.hpp"
#include "core/utils/log.h"

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
         * @param histogram_manager Pointer to the histogram manager
         */
        void load(Messenger* messenger,
                  ConfigManager* conf_manager,
                  GeometryManager* geo_manager,
                  HistogramManager* histogram_manager);

        /**
         * @brief Initialize all modules before the event sequence
         * @warning Should be called after the \ref ModuleManager::load "load function"
         */
        void initialize();

        /**
         * @brief Run all modules for the number of events
         * @param seeder Reference to the seeder
         * @param stop_token Stop token to interrupt the run
         * @warning Should be called after the \ref ModuleManager::initialize "init function"
         */
        void run(RandomNumberGenerator& seeder, const std::stop_token& stop_token);

        /**
         * @brief Pause the current simulation run
         * @details Halts the simulation after completing the current module step. Only has an effect during run.
         *
         * @param pause True to pause, false to resume
         */
        void pause(bool pause);

        /**
         * @brief Check the current pause status of the simulation
         * @return True if the simulation is paused, false otherwise
         */
        bool isPaused();

        /**
         * @brief Finalize all modules after the event sequence
         * @warning Should be called after the \ref ModuleManager::initialize "run function"
         */
        void finalize();

        /**
         * @brief Retrieve current event counts
         * @details This returns the total number of events to simulate, the number of currently finished events, the number
         * of events in the buffer and the number of aborted events, in this order.
         * @return Tuple with the number of total, finished, buffered and aborted events
         */
        std::tuple<std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t> getEventCounts() const;

        /**
         * @brief Static method to return the Allpix Squared version of this manager
         * @return Allpix Squared version
         */
        static std::string version();

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

        HistogramManager* histogram_manager_{};

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

        // Event counters
        std::atomic<uint64_t> events_total_;
        std::atomic<uint64_t> events_buffered_;
        std::atomic<uint64_t> events_finished_;
        std::atomic<uint64_t> events_aborted_;

        // User defined multithreading flags and parameters from configuration
        bool multithreading_flag_{false};
        unsigned int number_of_threads_{0};
        size_t max_buffer_size_{1};

        // Possibility of running loaded modules in parallel
        bool can_parallelize_{true};
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_MANAGER_H */
