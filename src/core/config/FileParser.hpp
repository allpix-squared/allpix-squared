/**
 * @file
 * @brief Provides a parser for configuration files
 *
 * @copyright Copyright (c) 2017-2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_FILE_PARSER_H
#define ALLPIX_FILE_PARSER_H

#include <filesystem>
#include <istream>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "ConfigStack.hpp"
#include "Configuration.hpp"

namespace allpix {

    /**
     * @brief Reader of configuration files
     *
     * Read the internal configuration file format used in the framework. The format contains
     * - A set of section header between [ and ] brackets
     * - Key/value pairs linked to the last defined section (or the empty section if none has been defined yet)
     */
    class FileParser {
    public:
        FileParser() = default;
        virtual ~FileParser() = default;

        /**
         * @brief Parses a configuration file and returns the content as ConifgStack
         * @param stream Stream to read configuration from
         * @param file_name Name of the file related to the stream or empty if not linked to a file
         */
        static ConfigStack getStack(std::istream& stream, std::filesystem::path file_name = "");

        /**
         * @brief Parse a line as key-value pair
         * @param line Line to interpret
         * @return Pair of the key and the value
         */
        static std::pair<std::string, std::string> parseKeyValue(std::string line);

        /// @{
        /**
         * @brief No copy
         */
        FileParser(const FileParser&) = delete;
        FileParser& operator=(const FileParser&) = delete;
        /// @}

        /// @{
        /**
         * @brief Use default move behavior
         */
        FileParser(FileParser&&) noexcept = default;
        FileParser& operator=(FileParser&&) noexcept = default;
        /// @}
    };
} // namespace allpix

#endif /* ALLPIX_FILE_PARSER_H */
