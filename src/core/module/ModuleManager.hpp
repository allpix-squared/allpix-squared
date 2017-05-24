/**
 * @file
 * @brief Loading and execution of all modules
 * @copyright MIT License
 */

#ifndef ALLPIX_MODULE_MANAGER_H
#define ALLPIX_MODULE_MANAGER_H

#include <atomic>
#include <list>
#include <map>
#include <memory>
#include <queue>

#include "Module.hpp"
#include "core/config/Configuration.hpp"
#include "core/utils/log.h"

namespace allpix {

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
         * @warning Should be called after the \ref AllPix::load "load function"
         */
        void init();

        /**
         * @brief Run all modules for the number of events
         * @warning Should be called after the \ref AllPix::init "init function"
         */
        void run();

        /**
         * @brief Finalize all modules after the event sequence
         * @warning Should be called after the \ref AllPix::init "run function"
         */
        void finalize();

        /**
         * @brief Terminates as soon as the current event is finished
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
        std::pair<ModuleIdentifier, Module*> create_unique_modules(void*, Configuration, Messenger*, GeometryManager*);
        /**
         * @brief Create detector modules
         * @param library Void pointer to the loaded library
         * @param config Configuration of the module
         * @param messenger Pointer to the messenger
         * @param geo_manager Pointer to the geometry manager
         * @return A list of all created detector modules and their identifiers
         */
        std::vector<std::pair<ModuleIdentifier, Module*>>
        create_detector_modules(void*, Configuration, Messenger*, GeometryManager*);

        /**
         * @brief Set module specific log setting before running init/run/finalize
         */
        std::tuple<LogLevel, LogFormat> set_module_before(const std::string& mod_name, const Configuration& config);
        /**
         * @brief Reset global log setting after running init/run/finalize
         */
        void set_module_after(std::tuple<LogLevel, LogFormat> prev);

        using ModuleList = std::list<std::unique_ptr<Module>>;
        using IdentifierToModuleMap = std::map<ModuleIdentifier, ModuleList::iterator>;

        ModuleList modules_;
        IdentifierToModuleMap id_to_module_;

        std::map<Module*, long double> module_execution_time_;

        Configuration global_config_;

        std::map<std::string, void*> loaded_libraries_;

        std::atomic<bool> terminate_;
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_MANAGER_H */
