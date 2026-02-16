/**
 * @file
 * @brief Interface to the main configuration and its normal and special sections
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_CONFIG_MANAGER_H
#define ALLPIX_CONFIG_MANAGER_H

#include <filesystem>
#include <set>
#include <string>
#include <vector>

#include "ConfigReader.hpp"
#include "Configuration.hpp"
#include "OptionParser.hpp"
#include "core/module/ModuleIdentifier.hpp"

namespace allpix {

    /**
     * @ingroup Managers
     * @brief Manager responsible for loading and providing access to the main configuration
     *
     * The main configuration is the single most important source of configuration. It is split up in:
     * - Global headers that are combined into a single global (not module specific) configuration
     * - Ignored headers that are not used at all (mainly useful for debugging)
     * - All other headers representing all modules that have to be instantiated by the ModuleManager
     *
     * Configuration sections are always case-sensitive.
     */
    class ConfigManager {
    public:
        /**
         * @brief Construct the configuration manager
         * @param file_name Path to the main configuration file
         * @param global List of sections representing the global configuration (excluding the empty header section)
         * @param ignore List of sections that should be ignored
         */
        explicit ConfigManager(std::filesystem::path file_name,
                               std::initializer_list<std::string> global = {},
                               std::initializer_list<std::string> ignore = {"Ignore"});
        /**
         * @brief Use default destructor
         */
        ~ConfigManager() = default;

        /// @{
        /**
         * @brief Copying the manager is not allowed
         */
        ConfigManager(const ConfigManager&) = delete;
        ConfigManager& operator=(const ConfigManager&) = delete;
        /// @}

        /// @{
        /**
         * @brief Use default move behaviour
         */
        ConfigManager(ConfigManager&&) noexcept = default;
        ConfigManager& operator=(ConfigManager&&) noexcept = default;
        /// @}

        /**
         * @brief Get the global configuration
         * @return Reference to global configuration
         *
         * The global configuration is the combination of all sections with a global header.
         */
        Configuration& getGlobalConfiguration() { return global_config_; }
        /**
         * @brief Get all the module configurations
         * @return Reference to list of module configurations
         *
         * All special global and ignored sections are not included in the list of module configurations.
         */
        std::list<Configuration>& getModuleConfigurations() { return module_configs_; }

        /**
         * @brief Add a new module instance configuration and applies instance options
         * @param identifier Identifier for this module instance
         * @param config Instance configuration to store
         * @return Reference to stored instance configuration
         */
        Configuration& addInstanceConfiguration(const ModuleIdentifier& identifier, const Configuration& config);
        /**
         * @brief Get all the instance configurations
         * @return Reference to list of instance configurations
         *
         * The list of instance configurations can contain configurations with duplicate names, but the instanceconfiguration
         * is guaranteed to have a configuration value 'identifier' that contains an unique identifier for every same config
         * name.
         */
        const std::list<Configuration>& getInstanceConfigurations() const { return instance_configs_; }

        /**
         * @brief Drops an instance configuration from instance configuration storage
         * @param identifier Identifier of the module instance to drop
         */
        void dropInstanceConfiguration(const ModuleIdentifier& identifier);

        /**
         * @brief Load module options and directly apply them to the global configuration and the module configurations
         * @param options List of options to load and apply
         * @return True if any actual options where applied
         *
         * @note Instance configuration options are applied in \ref ConfigManager::addInstanceConfiguration instead
         */
        bool loadModuleOptions(const std::vector<std::string>& options);
        /**
         * @brief Get all the detector configurations
         * @return Reference to list of detector configurations
         */
        std::list<Configuration>& getDetectorConfigurations();

        /**
         * @brief Load detector specific options
         * @param options List of options to load and apply
         * @return True if any actual options where applied
         */
        bool loadDetectorOptions(const std::vector<std::string>& options);

    private:
        std::set<std::string> global_names_;
        std::set<std::string> ignore_names_;

        OptionParser module_option_parser_;

        std::list<Configuration> module_configs_;
        Configuration global_config_;

        // Helper function for delayed parsing of detectors file
        void parse_detectors();
        std::list<Configuration> detector_configs_;

        std::list<Configuration> instance_configs_;
        std::map<ModuleIdentifier, std::list<Configuration>::iterator> instance_identifier_to_config_;
    };
} // namespace allpix

#endif /* ALLPIX_CONFIG_MANAGER_H */
