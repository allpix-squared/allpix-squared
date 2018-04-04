/**
 * @file
 * @brief Interface to the main configuration and its normal and special sections
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_CONFIG_MANAGER_H
#define ALLPIX_CONFIG_MANAGER_H

#include <set>
#include <string>
#include <vector>

#include "ConfigReader.hpp"
#include "Configuration.hpp"
#include "OptionParser.hpp"

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
        explicit ConfigManager(std::string file_name,
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
         */
        Configuration& getGlobalConfiguration();
        /**
         * @brief Get all the module configurations
         * @return Reference to list of module configurations
         */
        std::list<Configuration>& getModuleConfigurations();

        /**
         * @brief Add a new module instance configuration and applies instance options
         * @param config Instance configuration to store
         * @return Reference to stored instance configuration
         */
        Configuration& addInstanceConfiguration(std::string unique_identifier, Configuration config);
        /**
         * @brief Get all the instance configurations
         * @return Reference to list of instance configurations
         */
        // std::vector<Configuration>& getInstanceConfigurations();

        /**
         * @brief Get the option parser
         * @return Reference to the option parser
         */
        OptionParser& getOptionParser();
        /**
         * @brief Apply options to the global configuration and the module configurations
         * @return True if any actual options where applied
         *
         * @note Instance configuration options are applied in \ref ConfigManager::addInstanceConfiguration
         */
        bool loadOptions();

        /**
         * @brief Get all the detector configurations
         * @return Reference to list of detector configurations
         */
        // std::vector<Configuration>& getDetectorConfigurations();
    private:
        std::string file_name_;
        ConfigReader reader_;

        std::set<std::string> global_names_{};
        std::set<std::string> ignore_names_{};

        OptionParser option_parser_;

        std::list<Configuration> module_configs_;
        Configuration global_config_;

        std::list<Configuration> instance_configs_;
    };
} // namespace allpix

#endif /* ALLPIX_CONFIG_MANAGER_H */
