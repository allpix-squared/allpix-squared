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
         */
        explicit ConfigManager(std::string file_name);
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
         * @brief Set the name of the global header and add to the global names
         * @param name Name of a global header that should be used as the name
         */
        // TODO [doc] Should only set the name and do not add it
        void setGlobalHeaderName(std::string name);
        /**
         * @brief Add a global header name
         * @param name Name of a global header section
         */
        // TODO [doc] Rename to addGlobalHeader
        void addGlobalHeaderName(std::string name);

        /**
         * @brief Get the global configuration
         * @return Global configuration
         */
        Configuration getGlobalConfiguration();

        /**
         * @brief Add a header name to fully ignore
         * @param name Name of a header to ignore
         */
        // TODO [doc] Rename to ignoreHeader
        void addIgnoreHeaderName(std::string name);

        /**
         * @brief Parse an extra configuration option
         * @param line Line with the option
         */
        void parseOption(std::string line);

        /**
         * @brief Apply all relevant options to the passed configuration
         * @param identifier Identifier to select the options to apply
         * @param config Configuration option where the options should be applied to
         * @return True if the configuration was changed because of applied options
         */
        bool applyOptions(const std::string& identifier, Configuration& config);

        /**
         * @brief Return if section with given name exists
         * @param name Name of the section
         * @return True if at least one section with that name exists, false otherwise
         */
        bool hasConfiguration(const std::string&);

        /**
         * @brief Get all configurations that are not global or ignored
         * @return List of all normal configurations
         */
        std::vector<Configuration> getConfigurations() const;

    private:
        std::string file_name_;

        ConfigReader reader_;

        Configuration global_base_config_;
        std::string global_default_name_{};
        std::set<std::string> global_names_{};

        std::set<std::string> ignore_names_{};

        std::map<std::string, std::vector<std::pair<std::string, std::string>>> identifier_options_{};
    };
} // namespace allpix

#endif /* ALLPIX_CONFIG_MANAGER_H */
