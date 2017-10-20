/**
 * @file
 * @brief Implementation of configuration
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Configuration.hpp"

#include <cassert>
#include <ostream>
#include <stdexcept>
#include <string>

#include "core/utils/file.h"
#include "exceptions.h"

#include "core/utils/log.h"

using namespace allpix;

Configuration::Configuration(std::string name, std::string path) : name_(std::move(name)), path_(std::move(path)) {}

bool Configuration::has(const std::string& key) const {
    return config_.find(key) != config_.cend();
}

std::string Configuration::getName() const {
    return name_;
}
void Configuration::setName(const std::string& name) {
    name_ = name;
}
std::string Configuration::getFilePath() const {
    return path_;
}

std::string Configuration::getText(const std::string& key) const {
    try {
        // NOTE: returning literally including ""
        return config_.at(key);
    } catch(std::out_of_range& e) {
        throw MissingKeyError(key, getName());
    }
}
std::string Configuration::getText(const std::string& key, const std::string& def) const {
    if(!has(key)) {
        return def;
    }
    return getText(key);
}

/**
 * @throws InvalidValueError If the path did not exists while the check_exists parameter is given
 *
 * For a relative path the absolute path of the configuration file is preprended. Absolute paths are not changed.
 */
// TODO [doc] Document canonicalizing behaviour
std::string Configuration::getPath(const std::string& key, bool check_exists) const {
    try {
        return path_to_absolute(get<std::string>(key), check_exists);
    } catch(std::invalid_argument& e) {
        throw InvalidValueError(*this, key, e.what());
    }
}
/**
 * @throws InvalidValueError If the path did not exists while the check_exists parameter is given
 *
 * For all relative paths the absolute path of the configuration file is preprended. Absolute paths are not changed.
 */
// TODO [doc] Document canonicalizing behaviour
std::vector<std::string> Configuration::getPathArray(const std::string& key, bool check_exists) const {
    std::vector<std::string> path_array = getArray<std::string>(key);

    // Convert all paths to absolute
    try {
        for(auto& path : path_array) {
            path = path_to_absolute(path, check_exists);
        }
        return path_array;
    } catch(std::invalid_argument& e) {
        throw InvalidValueError(*this, key, e.what());
    }
}
/**
 * @throws std::invalid_argument If the path does not exists
 */
std::string Configuration::path_to_absolute(std::string path, bool canonicalize_path) const {
    // If not a absolute path, make it an absolute path
    if(path[0] != '/') {
        // Get base directory of config file
        std::string directory = path_.substr(0, path_.find_last_of('/'));

        // Set new path
        path = directory + "/" + path;

        // Normalize path only if we have to check if it exists
        // NOTE: This throws an error if the path does not exist
        if(canonicalize_path) {
            path = allpix::get_absolute_path(path);
        }
    }
    return path;
}

void Configuration::setText(const std::string& key, const std::string& val) {
    config_[key] = val;
}

/**
 *  The alias is only used if new key does not exist but old key does
 */
void Configuration::setAlias(const std::string& new_key, const std::string& old_key) {
    if(!has(old_key) || has(new_key)) {
        return;
    }
    try {
        config_[new_key] = config_.at(old_key);
    } catch(std::out_of_range& e) {
        throw MissingKeyError(old_key, getName());
    }
}

unsigned int Configuration::countSettings() const {
    return static_cast<unsigned int>(config_.size());
}

/**
 * All keys that are already defined earlier in this configuration are not changed.
 */
void Configuration::merge(const Configuration& other) {
    for(auto config_pair : other.config_) {
        // Only merge values that do not yet exist
        if(!has(config_pair.first)) {
            setText(config_pair.first, config_pair.second);
        }
    }
}

std::vector<std::pair<std::string, std::string>> Configuration::getAll() {
    std::vector<std::pair<std::string, std::string>> result;

    // Loop over all configuration keys
    for(auto& key_value : config_) {
        // Skip internal keys starting with an underscore
        if(!key_value.first.empty() && key_value.first[0] == '_') {
            continue;
        }

        result.emplace_back(key_value);
    }

    return result;
}

/**
 * Parse info ??
 */
std::unique_ptr<Configuration::parse_node> Configuration::parse_string(std::string str, int depth) {
    using parse_node = Configuration::parse_node;

    auto node = std::make_unique<parse_node>();
    str = allpix::trim(str);
    if(str.empty()) {
        throw std::invalid_argument("string is empty");
    }

    // Add pair of brackets if not there yet on the lowest depth
    if(depth == 0 && str.front() != '[') {
        str = '[' + str + ']';
    }

    // Check if value or list
    if(str.front() == '[' && str.back() == ']') {
        // Found list, recurse on items
        size_t lst = 1;
        int in_dpt = 0;
        for(size_t i = 1; i < str.size() - 1; ++i) {
            // Skip over quotation marks
            if(str[i] == '\'' || str[i] == '\"') {
                i = str.find(str[i], i + 1);
                continue;
            }

            // Handle brackets
            if(str[i] == '[')
                ++in_dpt;
            else if(str[i] == ']')
                --in_dpt;

            // Make subitems at the zero level
            if(in_dpt == 0 && str[i] == ',') {
                node->children.push_back(parse_string(str.substr(lst, i - lst), depth + 1));
                lst = i + 1;
            }
        }

        // Handle last item
        node->children.push_back(parse_string(str.substr(lst, str.size() - 1 - lst), depth + 1));
    } else {
        // Not an array, handle as value instead
        node->value = str;
    }

    return node;
}
