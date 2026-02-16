/**
 * @file
 * @brief Option parser for additional command line options
 *
 * @copyright Copyright (c) 2018-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_OPTION_PARSER_H
#define ALLPIX_OPTION_PARSER_H

#include <set>
#include <string>
#include <vector>

#include "Configuration.hpp"

namespace allpix {

    /**
     * @ingroup Managers
     * @brief Option parser responsible for parsing and caching command line arguments
     *
     * The option parser stores additional configuration items provided via the command line interface for later reference,
     * since most of the parameters can only be applied once all modules have been instantiated.
     */
    class OptionParser {
    public:
        /**
         * @brief Use default constructor
         */
        OptionParser() = default;
        /**
         * @brief Use default destructor
         */
        ~OptionParser() = default;

        /// @{
        /**
         * @brief Copying the option parser is not allowed
         */
        OptionParser(const OptionParser&) = delete;
        OptionParser& operator=(const OptionParser&) = delete;
        /// @}

        /// @{
        /**
         * @brief Use default move behaviour
         */
        OptionParser(OptionParser&&) noexcept = default; // NOLINT
        OptionParser& operator=(OptionParser&&) = default;
        /// @}

        /**
         * @brief Parse an extra configuration option
         * @param line Line with the option
         */
        void parseOption(std::string line);

        /**
         * @brief Apply all global options to a given global configuration object
         * @param config Global configuration option to which the options should be applied to
         * @return True if new global configuration options were applied
         */
        bool applyGlobalOptions(Configuration& config);

        /**
         * @brief Apply all relevant options to the given configuration object
         * @param identifier Identifier to select the options to apply
         * @param config Configuration option to which the options should be applied to
         * @return True if the configuration was changed because of applied options
         */
        bool applyOptions(const std::string& identifier, Configuration& config);

    private:
        std::vector<std::pair<std::string, std::string>> global_options_;

        std::map<std::string, std::vector<std::pair<std::string, std::string>>> identifier_options_;
    };
} // namespace allpix

#endif /* ALLPIX_OPTION_PARSER_H */
