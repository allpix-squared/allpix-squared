/**
 * @file
 * @brief Implementation of config reader
 *
 * @copyright Copyright (c) 2017-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "ConfigReader.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "core/config/exceptions.h"
#include "core/utils/log.h"

using namespace allpix;

ConfigReader::ConfigReader() = default;
ConfigReader::ConfigReader(std::istream& stream, std::filesystem::path file_name) : ConfigReader() {
    add(stream, std::move(file_name));
}

ConfigReader::ConfigReader(const ConfigReader& other) : conf_array_(other.conf_array_) {
    copy_init_map();
}
ConfigReader& ConfigReader::operator=(const ConfigReader& other) {
    conf_array_ = other.conf_array_;
    copy_init_map();
    return *this;
}

void ConfigReader::copy_init_map() {
    conf_map_.clear();
    for(auto iter = conf_array_.begin(); iter != conf_array_.end(); ++iter) {
        conf_map_[allpix::transform(iter->getName(), ::tolower)].push_back(iter);
    }
}

/**
 * @throws KeyValueParseError If the key / value pair could not be parsed
 *
 * The key / value pair is split according to the format specifications
 */
std::pair<std::string, std::string> ConfigReader::parseKeyValue(std::string line) {
    line = allpix::trim(line);
    size_t equals_pos = line.find('=');
    if(equals_pos != std::string::npos) {
        std::string key = trim(std::string(line, 0, equals_pos));
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
void ConfigReader::add(std::istream& stream, std::filesystem::path file_name) {
    LOG(TRACE) << "Parsing configuration file " << file_name;

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
                    addConfiguration(std::move(conf));
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
    addConfiguration(std::move(conf));
}

void ConfigReader::addConfiguration(Configuration config) {
    conf_array_.push_back(std::move(config));

    auto section_name = allpix::transform(conf_array_.back().getName(), ::tolower);
    conf_map_[section_name].push_back(--conf_array_.end());
}

void ConfigReader::clear() {
    conf_map_.clear();
    conf_array_.clear();
}

bool ConfigReader::hasConfiguration(std::string name) const {
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    return conf_map_.find(name) != conf_map_.end();
}

unsigned int ConfigReader::countConfigurations(std::string name) const {
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if(!hasConfiguration(name)) {
        return 0;
    }
    return static_cast<unsigned int>(conf_map_.at(name).size());
}

/**
 * @warning This will have the file path of the first header section
 * @note An empty configuration is returned if no empty section is found
 */
Configuration ConfigReader::getHeaderConfiguration() const {
    // Get empty configurations
    std::vector<Configuration> configurations = getConfigurations("");
    if(configurations.empty()) {
        // Use all configurations to find the file name if no empty
        configurations = getConfigurations();
        std::string file_name;
        if(!configurations.empty()) {
            file_name = configurations.at(0).getFilePath();
        }
        return Configuration("", file_name);
    }

    // Merge all configurations
    Configuration header_config = configurations.at(0);
    for(auto& config : configurations) {
        // NOTE: Merging first configuration again has no effect
        header_config.merge(config);
    }
    return header_config;
}

std::vector<Configuration> ConfigReader::getConfigurations(std::string name) const {
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if(!hasConfiguration(name)) {
        return std::vector<Configuration>();
    }

    std::vector<Configuration> result;
    for(const auto& iter : conf_map_.at(name)) {
        result.push_back(*iter);
    }
    return result;
}

std::vector<Configuration> ConfigReader::getConfigurations() const {
    return std::vector<Configuration>(conf_array_.begin(), conf_array_.end());
}
