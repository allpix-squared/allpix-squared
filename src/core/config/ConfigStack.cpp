/**
 * @file
 * @brief Implementation of config stack
 *
 * @copyright Copyright (c) 2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "ConfigStack.hpp"

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

ConfigStack::ConfigStack(const ConfigStack& other) : conf_array_(other.conf_array_) { copy_init_map(); }
ConfigStack& ConfigStack::operator=(const ConfigStack& other) {
    conf_array_ = other.conf_array_;
    copy_init_map();
    return *this;
}

void ConfigStack::copy_init_map() {
    conf_map_.clear();
    for(auto iter = conf_array_.begin(); iter != conf_array_.end(); ++iter) {
        conf_map_[allpix::transform(iter->getName(), ::tolower)].push_back(iter);
    }
}

void ConfigStack::addConfiguration(Configuration config) {
    conf_array_.push_back(std::move(config));

    auto section_name = allpix::transform(conf_array_.back().getName(), ::tolower);
    conf_map_[section_name].push_back(--conf_array_.end());
}

void ConfigStack::clear() {
    conf_map_.clear();
    conf_array_.clear();
}

bool ConfigStack::hasConfiguration(std::string name) const {
    std::ranges::transform(name, name.begin(), ::tolower);
    return conf_map_.find(name) != conf_map_.end();
}

unsigned int ConfigStack::countConfigurations(std::string name) const {
    std::ranges::transform(name, name.begin(), ::tolower);
    if(!hasConfiguration(name)) {
        return 0;
    }
    return static_cast<unsigned int>(conf_map_.at(name).size());
}

/**
 * @warning This will have the file path of the first header section
 * @note An empty configuration is returned if no empty section is found
 */
Configuration ConfigStack::getHeaderConfiguration() const {
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

std::vector<Configuration> ConfigStack::getConfigurations(std::string name) const {
    std::ranges::transform(name, name.begin(), ::tolower);
    if(!hasConfiguration(name)) {
        return {};
    }

    std::vector<Configuration> result;
    for(const auto& iter : conf_map_.at(name)) {
        result.push_back(*iter);
    }
    return result;
}

std::vector<Configuration> ConfigStack::getConfigurations() const { return {conf_array_.begin(), conf_array_.end()}; }
