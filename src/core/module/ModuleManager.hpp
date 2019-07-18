/**
 * @file
 * @brief Loading and execution of all modules
 * @copyright Copyright (c) 2017-2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MODULE_MANAGER_H
#define ALLPIX_MODULE_MANAGER_H

#include <atomic>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <random>

#include <TDirectory.h>
#include <TFile.h>

#include "Module.hpp"
#include "ThreadPool.hpp"
#include "core/config/Configuration.hpp"
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
         */
        void load(Messenger* messenger, ConfigManager* conf_manager, GeometryManager* geo_manager);

        /**
         * @brief Initialize all modules before the event sequence
         * @param seeder Reference to the seeder
         * @warning Should be called after the \ref ModuleManager::load "load function"
         */
        void init(std::mt19937_64& seeder);

        /**
         * @brief Run all modules for the number of events
         * @param seeder Reference to the seeder
         * @warning Should be called after the \ref ModuleManager::init "init function"
         */
        void run(std::mt19937_64& seeder);

        /**
         * @brief Finalize all modules after the event sequence
         * @warning Should be called after the \ref ModuleManager::init "run function"
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
         * @param seeder Seeder used to construct the PRNG of the modules
         * @return An unique module together with its identifier
         */
        std::pair<ModuleIdentifier, Module*> create_unique_modules(void*, Configuration&, Messenger*, GeometryManager*);

        /**
         * @brief Create detector modules
         * @param library Void pointer to the loaded library
         * @param config Configuration of the module
         * @param messenger Pointer to the messenger
         * @param geo_manager Pointer to the geometry manager
         * @param seeder Seeder used to construct the PRNG of the modules
         * @return A list of all created detector modules and their identifiers
         */
        std::vector<std::pair<ModuleIdentifier, Module*>>
        create_detector_modules(void*, Configuration&, Messenger*, GeometryManager*);

        /**
         * @brief Set module specific log setting before running init/run/finalize
         */
        static std::tuple<LogLevel, LogFormat> set_module_before(const std::string& mod_name, const Configuration& config);
        /**
         * @brief Reset global log setting after running init/run/finalize
         */
        static void set_module_after(std::tuple<LogLevel, LogFormat> prev);

        using IdentifierToModuleMap = std::map<ModuleIdentifier, ModuleList::iterator>;

        ModuleList modules_;
        IdentifierToModuleMap id_to_module_;

        ConfigManager* conf_manager_;

        std::unique_ptr<TFile> modules_file_;

        std::map<Module*, long double> module_execution_time_;
        long double total_time_{};

        std::map<std::string, void*> loaded_libraries_;

        std::atomic<bool> terminate_;

        Messenger* messenger_;
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_MANAGER_H */
