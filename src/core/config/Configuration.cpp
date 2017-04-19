/**
 *  @author Simon Spannagel <simon.spannagel@cern.ch>
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "Configuration.hpp"

#include <cassert>
#include <ostream>
#include <stdexcept>
#include <string>

#include "InvalidValueError.hpp"
#include "core/utils/file.h"
#include "exceptions.h"

#include <iostream>

using namespace allpix;

// Constructors
Configuration::Configuration() : Configuration("") {}
Configuration::Configuration(std::string name) : Configuration(std::move(name), "") {}
Configuration::Configuration(std::string name, std::string path)
    : name_(std::move(name)), path_(std::move(path)), config_() {}

// Check if key exists
bool Configuration::has(const std::string& key) const {
    return config_.find(key) != config_.cend();
}

// Get name of configuration
std::string Configuration::getName() const {
    return name_;
}
// Get path of configuration
std::string Configuration::getFilePath() const {
    return path_;
}

// Get as text
std::string Configuration::getText(const std::string& key) const {
    try {
        // NOTE: returning literally including ""
        return config_.at(key);
    } catch(std::out_of_range& e) {
        throw MissingKeyError(key, getName());
    }
}
// Get text with default
std::string Configuration::getText(const std::string& key, const std::string& def) const {
    if(!has(key)) {
        return def;
    }
    return getText(key);
}

// Get path with relative fixed to absolute paths
std::string Configuration::getPath(const std::string& key, bool check_exists) const {
    try {
        return path_to_absolute(get<std::string>(key), check_exists);
    } catch(std::invalid_argument& e) {
        throw InvalidValueError(*this, key, e.what());
    }
}
// Get paths as array
std::vector<std::string> Configuration::getPathArray(const std::string& key, bool check_exists) const {
    std::vector<std::string> path_array = getArray<std::string>(key);

    // convert all paths to absolute
    try {
        for(auto& path : path_array) {
            path = path_to_absolute(path, check_exists);
        }
        return path_array;
    } catch(std::invalid_argument& e) {
        throw InvalidValueError(*this, key, e.what());
    }
}
// Convert path to absolute (normalizes if necessary - in that case it throws an error if it does not exists)
std::string Configuration::path_to_absolute(std::string path, bool normalize_path) const {
    // NOTE: allow this function to be called only for valid paths
    assert(!path.empty());
    // if not a absolute path, make it an absolute path
    if(path[0] != '/') {
        // get base directory of config file
        std::string directory = path_.substr(0, path_.find_last_of('/'));

        // update path
        path = directory + "/" + path;

        // normalize path only if we have to check if it exists
        // NOTE: this throws an error if the path does not exist
        if(normalize_path) {
            path = get_absolute_path(path);
        }
    }
    return path;
}

// Set literal text
void Configuration::setText(const std::string& key, const std::string& val) {
    config_[key] = val;
}

// Count amount of settings
unsigned int Configuration::countSettings() const {
    return static_cast<unsigned int>(config_.size());
}

// Merge configuration into each other
void Configuration::merge(const Configuration& other) {
    for(auto config_pair : other.config_) {
        // only merge values that do not yet exist
        if(!has(config_pair.first)) {
            setText(config_pair.first, config_pair.second);
        }
    }
}

void Configuration::print(std::ostream& out) const {
    for(auto& element : config_) {
        out << element.first << " : " << element.second << std::endl;
    }
}
