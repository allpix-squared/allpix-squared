/**
 * @file
 * @brief Implementation of config manager
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "ConfigManager.hpp"

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

#include "Configuration.hpp"
#include "core/utils/file.h"
#include "core/utils/log.h"
#include "exceptions.h"

using namespace allpix;

/**
 * @throws ConfigFileUnavailableError If the main configuration file cannot be accessed
 */
ConfigManager::ConfigManager(std::string file_name) : file_name_(std::move(file_name)) {
    LOG(TRACE) << "Using " << file_name_ << " as main configuration file";

    // Check if the file exists
    std::ifstream file(file_name_);
    if(!file) {
        throw ConfigFileUnavailableError(file_name_);
    }

    // Convert main file to absolute path
    file_name_ = allpix::get_absolute_path(file_name_);

    // Initialize global base configuration with absolute file name
    global_base_config_ = Configuration("", file_name_);

    // Read the file
    reader_.add(file, file_name_);
}

/**
 * @warning Only one header can be added in this way to define its main name
 */
void ConfigManager::setGlobalHeaderName(std::string name) {
    global_default_name_ = name;
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    global_names_.emplace(std::move(name));
}
void ConfigManager::addGlobalHeaderName(std::string name) {
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    global_names_.emplace(std::move(name));
}

/**
 * The global configuration is the combination of all sections with a global header.
 */
Configuration ConfigManager::getGlobalConfiguration() {
    // Copy base config and set name
    Configuration global_config = global_base_config_;
    global_config.setName(global_default_name_);

    // Add all other global configuration
    for(auto& global_name : global_names_) {
        auto configs = reader_.getConfigurations(global_name);
        for(auto& config : configs) {
            global_config.merge(config);
        }
    }
    return global_config;
}
void ConfigManager::addIgnoreHeaderName(std::string name) {
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    ignore_names_.emplace(std::move(name));
}

/**
 * Option is split in a key / value pair, an error is thrown if that is not possible. When the key contains at least one dot
 * it is interpreted as a relative configuration with the module identified by the first dot. In that case the option is
 * applied during module loading when either the unique or the configuration name match. Otherwise the key is interpreted as
 * global key and is added to the global header.
 */
void ConfigManager::parseOption(std::string line) {
    line = allpix::trim(line);
    auto key_value = ConfigReader::parseKeyValue(line);
    auto key = key_value.first;
    auto value = key_value.second;

    auto dot_pos = key.find('.');
    if(dot_pos == std::string::npos) {
        // Global option, add to the global base config
        global_base_config_.setText(key, value);
    } else {
        // Other identifier bound option is passed
        auto identifier = key.substr(0, dot_pos);
        key = key.substr(dot_pos + 1);
        identifier_options_[identifier].push_back(std::make_pair(key, value));
    }
}

bool ConfigManager::applyOptions(const std::string& identifier, Configuration& config) {
    if(identifier_options_.find(identifier) == identifier_options_.end()) {
        return false;
    }

    for(auto& key_value : identifier_options_[identifier]) {
        config.setText(key_value.first, key_value.second);
    }
    return true;
}

bool ConfigManager::hasConfiguration(const std::string& name) {
    return reader_.hasConfiguration(name);
}

/**
 * All special global and ignored sections are removed before returning the rest of the configurations. The list of normal
 * sections is used by the ModuleManager to instantiate all the required modules.
 */
std::vector<Configuration> ConfigManager::getConfigurations() const {
    std::vector<Configuration> result;
    for(auto& config : reader_.getConfigurations()) {
        // ignore all global and ignores names
        std::string config_name = config.getName();
        std::transform(config_name.begin(), config_name.end(), config_name.begin(), ::tolower);
        if(global_names_.find(config_name) != global_names_.end() ||
           ignore_names_.find(config_name) != ignore_names_.end()) {
            continue;
        }
        result.push_back(config);
    }

    return result;
}
