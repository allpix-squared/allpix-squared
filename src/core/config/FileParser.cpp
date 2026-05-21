/**
 * @file
 * @brief Implementation of config file parser
 *
 * @copyright Copyright (c) 2017-2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "FileParser.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <istream>
#include <string>
#include <utility>
#include <vector>

#include "core/config/exceptions.h"
#include "core/utils/log.h"
#include "core/utils/text.h"

using namespace allpix;

/**
 * @throws KeyValueParseError If the key / value pair could not be parsed
 *
 * The key / value pair is split according to the format specifications
 */
std::pair<std::string, std::string> FileParser::parseKeyValue(std::string line) {
    line = allpix::trim(line);
    size_t const equals_pos = line.find('=');
    if(equals_pos != std::string::npos) {
        std::string const key = trim(std::string(line, 0, equals_pos));
        std::string value = trim(std::string(line, equals_pos + 1));
        char last_quote = 0;
        for(size_t i = 0; i < value.size(); ++i) {
            if(value[i] == '\'' || value[i] == '\"') {
                if(last_quote == 0) {
                    last_quote = value[i];
                } else if(last_quote == value[i]) {
                    last_quote = 0;
                }
            }
            if(last_quote == 0 && value[i] == '#') {
                value = std::string(value, 0, i);
                break;
            }
        }

        // Check if key contains only alphanumeric or underscores
        bool valid_key = true;
        for(auto& ch : key) {
            if(isalnum(ch) == 0 && ch != '_' && ch != '.' && ch != ':') {
                valid_key = false;
                break;
            }
        }

        // Check if value is not empty and key is valid
        if(!valid_key) {
            throw KeyValueParseError(line, "key is not valid");
        }
        if(value.empty()) {
            throw KeyValueParseError(line, "value is empty");
        }

        return std::make_pair(key, allpix::trim(value));
    }
    // Key / value pair does not contain equal sign
    throw KeyValueParseError(line, "missing equality sign to split key and value");
}

/**
 * @throws ConfigParseError If an error occurred during the parsing of the stream
 *
 * The configuration is immediately parsed and all of its configurations are available after the functions returns.
 */
ConfigStack FileParser::getStack(std::istream& stream, std::filesystem::path file_name) {
    LOG(TRACE) << "Parsing configuration file " << file_name;
    ConfigStack stack;

    // Convert file name to absolute path (if given)
    if(!file_name.empty()) {
        file_name = std::filesystem::canonical(file_name);
    }

    // Build first empty configuration
    std::string section_name;
    Configuration conf(section_name, file_name);

    int line_num = 0;
    while(true) {
        // Process config line by line
        std::string line;
        if(stream.eof()) {
            break;
        }
        std::getline(stream, line);
        ++line_num;

        // Trim whitespaces at beginning and end of line:
        line = allpix::trim(line);

        // Ignore empty lines or comments
        if(line.empty() || line.front() == '#') {
            continue;
        }

        // Check if section header or key-value pair
        if(line.front() == '[') {
            // Line should be a section header with an alphanumeric name
            size_t idx = 1;
            for(; idx < line.length() - 1; ++idx) {
                if(isalnum(line[idx]) == 0 && line[idx] != '_') {
                    break;
                }
            }
            std::string remain = allpix::trim(line.substr(idx + 1));
            if(line[idx] == ']' && (remain.empty() || remain.front() == '#')) {
                // Ignore empty sections if they contain no configurations
                if(!conf.getName().empty() || conf.countSettings() > 0) {
                    // Add previous section
                    stack.addConfiguration(std::move(conf));
                }

                // Begin new section
                section_name = std::string(line, 1, idx - 1);
                conf = Configuration(section_name, file_name);
            } else {
                // Section header is not valid
                throw ConfigParseError(file_name, line_num);
            }
        } else if(isalpha(line.front()) != 0) {
            // Line should be a key / value pair with an equal sign
            try {
                // Parse the key value pair
                auto [key, value] = parseKeyValue(std::move(line));

                // Add the config key
                conf.setText(key, value);
            } catch(KeyValueParseError& e) {
                // Rethrow key / value parse error as a configuration parse error
                throw ConfigParseError(file_name, line_num);
            }
        } else {
            // Line is not a comment, key/value pair or section header
            throw ConfigParseError(file_name, line_num);
        }
    }
    // Add last section
    stack.addConfiguration(std::move(conf));

    return stack;
}
