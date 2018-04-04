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
ConfigManager::ConfigManager(std::string file_name,
                             std::initializer_list<std::string> global,
                             std::initializer_list<std::string> ignore)
    : file_name_(std::move(file_name)) {
    // Check if the file exists
    std::ifstream file(file_name_);
    if(!file) {
        throw ConfigFileUnavailableError(file_name_);
    }

    // Convert main file to absolute path
    file_name_ = allpix::get_canonical_path(file_name_);
    LOG(TRACE) << "Using " << file_name_ << " as main configuration file";

    // Read the file
    reader_.add(file, file_name_);

    // Convert all global and ignored names to lower case and store them
    auto lowercase = [](const std::string& in) {
        std::string out(in);
        std::transform(out.begin(), out.end(), out.begin(), ::tolower);
        return out;
    };
    std::transform(global.begin(), global.end(), std::inserter(global_names_, global_names_.end()), lowercase);
    std::transform(ignore.begin(), ignore.end(), std::inserter(ignore_names_, ignore_names_.end()), lowercase);

    // Initialize global base configuration
    global_config_ = reader_.getHeaderConfiguration();

    // Store all the configurations read
    for(auto& config : reader_.getConfigurations()) {
        // Skip all ignored sections
        std::string config_name = config.getName();
        std::transform(config_name.begin(), config_name.end(), config_name.begin(), ::tolower);
        if(ignore_names_.find(config_name) != ignore_names_.end()) {
            continue;
        }

        // Merge all global section with the global config
        if(global_names_.find(config_name) != global_names_.end()) {
            global_config_.merge(config);
            continue;
        }

        module_configs_.push_back(config);
    }
}

/**
 * The global configuration is the combination of all sections with a global header.
 */
Configuration& ConfigManager::getGlobalConfiguration() {
    return global_config_;
}

/**
 * The option parser is used to apply specific settings on top of the default
 */
OptionParser& ConfigManager::getOptionParser() {
    return option_parser_;
}
/**
 * Load all the options that are added to the option parser before calling this function.
 */
bool ConfigManager::loadOptions() {
    bool retValue = false;

    // Apply global options
    retValue = retValue || option_parser_.applyGlobalOptions(global_config_);

    // Apply module options
    for(auto& config : module_configs_) {
        retValue = retValue || option_parser_.applyOptions(config.getName(), config);
    }

    return retValue;
}

/**
 * All special global and ignored sections are not included in the list of module configurations.
 */
std::list<Configuration>& ConfigManager::getModuleConfigurations() {
    return module_configs_;
}

/**
 * An instance configuration is a specialized configuration for a particular module instance
 */
Configuration& ConfigManager::addInstanceConfiguration(std::string unique_name, Configuration config) {
    // Add configuration
    instance_configs_.push_back(config);
    Configuration& ret_config = instance_configs_.back();

    // Add unique identifier key
    ret_config.set<std::string>("unique_name", unique_name);

    // Apply instance options
    getOptionParser().applyOptions(unique_name, ret_config);
    return ret_config;
}

/**
 * The list of instance configurations can contain configurations with duplicate names, but the instance configuration is
 * guaranteed to have a configuration value 'unique_name' that contains an unique name
 */
std::list<Configuration>& ConfigManager::getInstanceConfigurations() {
    return instance_configs_;
}
