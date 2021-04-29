/**
 * @file
 * @brief Implementation of configuration
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Configuration.hpp"

#include <cassert>
#include <filesystem>
#include <ostream>
#include <stdexcept>
#include <string>

#include "core/config/exceptions.h"
#include "core/utils/log.h"

using namespace allpix;

Configuration::Configuration(std::string name, std::string path) : name_(std::move(name)), path_(std::move(path)) {}

bool Configuration::has(const std::string& key) const {
    return config_.find(key) != config_.cend();
}

unsigned int Configuration::count(std::initializer_list<std::string> keys) const {
    if(keys.size() == 0) {
        throw std::invalid_argument("list of keys cannot be empty");
    }

    unsigned int found = 0;
    for(const auto& key : keys) {
        if(has(key)) {
            found++;
        }
    }
    return found;
}

std::string Configuration::getName() const {
    return name_;
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
 * For a relative path the absolute path of the configuration file is prepended. Absolute paths are not changed.
 */
std::string
Configuration::getPathWithExtension(const std::string& key, const std::string& extension, bool check_exists) const {
    try {
        return path_to_absolute(std::filesystem::path(get<std::string>(key)).replace_extension(extension), check_exists);
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
    if(path.front() != '/') {
        // Get base directory of config file
        std::string directory = path_.substr(0, path_.find_last_of('/'));

        // Set new path
        path = directory + "/" + path;
    }

    // Normalize path only if we have to check if it exists
    // NOTE: This throws an error if the path does not exist
    if(canonicalize_path) {
        try {
            path = std::filesystem::canonical(path);
        } catch(std::filesystem::filesystem_error&) {
            throw std::invalid_argument("path " + path + " not found");
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
void Configuration::setAlias(const std::string& new_key, const std::string& old_key, bool warn) {
    if(!has(old_key) || has(new_key)) {
        return;
    }
    try {
        config_[new_key] = config_.at(old_key);
    } catch(std::out_of_range& e) {
        throw MissingKeyError(old_key, getName());
    }

    if(warn) {
        LOG(WARNING) << "Parameter \"" << old_key << "\" is deprecated and superseded by \"" << new_key << "\"";
    }
}

unsigned int Configuration::countSettings() const {
    return static_cast<unsigned int>(config_.size());
}

/**
 * All keys that are already defined earlier in this configuration are not changed.
 */
void Configuration::merge(const Configuration& other) {
    for(const auto& [key, value] : other.config_) {
        // Only merge values that do not yet exist
        if(!has(key)) {
            setText(key, value);
        }
    }
}

std::vector<std::pair<std::string, std::string>> Configuration::getAll() const {
    std::vector<std::pair<std::string, std::string>> result;

    // Loop over all configuration keys
    for(const auto& key_value : config_) {
        // Skip internal keys starting with an underscore
        if(!key_value.first.empty() && key_value.first.front() == '_') {
            continue;
        }

        result.emplace_back(key_value);
    }

    return result;
}

std::vector<std::string> Configuration::getUnusedKeys() const {
    std::vector<std::string> result;

    // Loop over all configuration keys, excluding internal ones
    for(const auto& key_value : getAll()) {
        // Add those to result that have not been accessed:
        if(used_keys_.find(key_value.first) == used_keys_.end() || !used_keys_.at(key_value.first)) {
            result.emplace_back(key_value.first);
        }
    }

    return result;
}

/**
 * String is recursively parsed for all pair of [ and ] brackets. All parts between single or double quotation marks are
 * skipped.
 */
std::unique_ptr<Configuration::parse_node> Configuration::parse_value(std::string str, int depth) {

    auto node = std::make_unique<parse_node>();
    str = allpix::trim(str);
    if(str.empty()) {
        throw std::invalid_argument("element is empty");
    }

    // Initialize variables for non-zero levels
    size_t beg = 1, lst = 1, in_dpt = 0;
    bool in_dpt_chg = false;

    // Implicitly add pair of brackets on zero level
    if(depth == 0) {
        beg = lst = 0;
        in_dpt = 1;
    }

    for(size_t i = 0; i < str.size(); ++i) {
        // Skip over quotation marks
        if(str[i] == '\'' || str[i] == '\"') {
            i = str.find(str[i], i + 1);
            if(i == std::string::npos) {
                throw std::invalid_argument("quotes are not balanced");
            }
            continue;
        }

        // Handle brackets
        if(str[i] == '[') {
            ++in_dpt;
            if(!in_dpt_chg && i != 0) {
                throw std::invalid_argument("invalid start bracket");
            }
            in_dpt_chg = true;
        } else if(str[i] == ']') {
            if(in_dpt == 0) {
                throw std::invalid_argument("brackets are not matched");
            }
            --in_dpt;
            in_dpt_chg = true;
        }

        // Make subitems at the zero level
        if(in_dpt == 1 && (str[i] == ',' || (isspace(str[i]) != 0 && (isspace(str[i - 1]) == 0 && str[i - 1] != ',')))) {
            node->children.push_back(parse_value(str.substr(lst, i - lst), depth + 1));
            lst = i + 1;
        }
    }

    if((depth > 0 && in_dpt != 0) || (depth == 0 && in_dpt != 1)) {
        throw std::invalid_argument("brackets are not balanced");
    }

    // Determine if array or value
    if(in_dpt_chg || depth == 0) {
        // Handle last array item
        size_t end = str.size();
        if(depth != 0) {
            if(str.back() != ']') {
                throw std::invalid_argument("invalid end bracket");
            }
            end = str.size() - 1;
        }
        node->children.push_back(parse_value(str.substr(lst, end - lst), depth + 1));
        node->value = str.substr(beg, end - beg);
    } else {
        // Not an array, handle as value instead
        node->value = str;
    }

    // Handle zero level where brackets where explicitly added
    if(depth == 0 && node->children.size() == 1 && !node->children.front()->children.empty()) {
        node = std::move(node->children.front());
    }

    return node;
}
