/**
 * @file
 * @brief Provides a reader for configuration files
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_CONFIG_READER_H
#define ALLPIX_CONFIG_READER_H

#include <filesystem>
#include <istream>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "Configuration.hpp"

namespace allpix {

    /**
     * @brief Reader of configuration files
     *
     * Read the internal configuration file format used in the framework. The format contains
     * - A set of section header between [ and ] brackets
     * - Key/value pairs linked to the last defined section (or the empty section if none has been defined yet)
     */
    class ConfigReader {
    public:
        /**
         * @brief Constructs a config reader without any attached streams
         */
        ConfigReader();
        /**
         * @brief Constructs a config reader with a single attached stream
         * @param stream Stream to read configuration from
         * @param file_name Name of the file related to the stream or empty if not linked to a file
         */
        explicit ConfigReader(std::istream& stream, std::filesystem::path file_name = "");

        /**
         * @brief Parse a line as key-value pair
         * @param line Line to interpret
         * @return Pair of the key and the value
         */
        static std::pair<std::string, std::string> parseKeyValue(std::string line);

        /**
         * @brief Adds a configuration stream to read
         * @param stream Stream to read configuration from
         * @param file_name Name of the file related to the stream or empty if not linked to a file
         */
        void add(std::istream&, std::filesystem::path file_name = "");

        /**
         * @brief Directly add a configuration object to the reader
         * @param config Configuration object to add
         */
        void addConfiguration(Configuration config);

        /// @{
        /**
         * @brief Implement correct copy behaviour
         */
        ConfigReader(const ConfigReader&);
        ConfigReader& operator=(const ConfigReader&);
        /// @}

        /// @{
        /**
         * @brief Use default move behaviour
         */
        ConfigReader(ConfigReader&&) noexcept = default;
        ConfigReader& operator=(ConfigReader&&) noexcept = default;
        /// @}

        /**
         * @brief Removes all streams and all configurations
         */
        void clear();

        /**
         * @brief Check if a configuration exists
         * @param name Name of a configuration header to search for
         * @return True if at least a single configuration with this name exists, false otherwise
         */
        bool hasConfiguration(std::string name) const;
        /**
         * @brief Count the number of configurations with a particular name
         * @param name Name of a configuration header
         * @return The number of configurations with the given name
         */
        unsigned int countConfigurations(std::string name) const;

        /**
         * @brief Get combined configuration of all empty sections (usually the header)
         * @note Typically this is only the section at the top of the file
         * @return Configuration object for the empty section
         */
        Configuration getHeaderConfiguration() const;

        /**
         * @brief Get all configurations with a particular header
         * @param name Header name of the configurations to return
         * @return List of configurations with the given name
         */
        std::vector<Configuration> getConfigurations(std::string name) const;

        /**
         * @brief Get all configurations
         * @return List of all configurations
         */
        std::vector<Configuration> getConfigurations() const;

    private:
        /**
         * @brief Initialize the configuration map after copy of the class
         */
        void copy_init_map();

        std::map<std::string, std::vector<std::list<Configuration>::iterator>> conf_map_;
        std::list<Configuration> conf_array_;
    };
} // namespace allpix

#endif /* ALLPIX_CONFIG_MANAGER_H */
