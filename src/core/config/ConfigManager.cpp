/**
 * @file
 * @brief Implementation of config manager
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "ConfigManager.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <list>
#include <string>
#include <utility>
#include <vector>

#include "core/config/Configuration.hpp"
#include "core/config/exceptions.h"
#include "core/utils/log.h"
#include "core/utils/text.h"

using namespace allpix;

/**
 * @throws ConfigFileUnavailableError If the main configuration file cannot be accessed
 */
ConfigManager::ConfigManager(Configuration header,
                             const std::vector<Configuration>& modules,
                             std::initializer_list<std::string> global,
                             std::initializer_list<std::string> ignore)
    : global_config_(std::move(header)) {

    // Convert all global and ignored names to lower case and store them
    auto lowercase = [](const std::string& in) { return allpix::transform(in, ::tolower); };
    std::transform(global.begin(), global.end(), std::inserter(global_names_, global_names_.end()), lowercase);
    std::transform(ignore.begin(), ignore.end(), std::inserter(ignore_names_, ignore_names_.end()), lowercase);

    // Store all module configurations
    for(const auto& config : modules) {
        // Skip all ignored sections
        std::string const config_name = allpix::transform(config.getName(), ::tolower);
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
 * Load the detector configurations
 */
void ConfigManager::loadDetectors(const std::vector<Configuration>& detector_configs) {
    detector_configs_ = std::list<Configuration>(detector_configs.begin(), detector_configs.end());
}

/**
 * Load all extra options that should be added on top of the configuration in the file. The options loaded here are
 * automatically applied to the module instance when these are added later.
 */
bool ConfigManager::loadModuleOptions(const std::vector<std::string>& options) {
    bool optionsApplied = false;

    // Parse the options
    for(const auto& option : options) {
        module_option_parser_.parseOption(option);
    }

    // Apply global options
    optionsApplied = module_option_parser_.applyGlobalOptions(global_config_) || optionsApplied;

    // Apply module options
    for(auto& config : module_configs_) {
        optionsApplied = module_option_parser_.applyOptions(config.getName(), config) || optionsApplied;
    }

    return optionsApplied;
}

/**
 * Load all extra options that should be added on top of the detector configuration in the file. The options loaded here are
 * automatically applied to the detector instance when these are added later and will be taken into account when possibly
 * loading customized detector models.
 */
bool ConfigManager::loadDetectorOptions(const std::vector<std::string>& options) {
    bool optionsApplied = false;

    // Create the parser
    OptionParser detector_option_parser;

    // Parse the options
    for(const auto& option : options) {
        detector_option_parser.parseOption(option);
    }

    // Apply detector options
    for(auto& config : detector_configs_) {
        optionsApplied = detector_option_parser.applyOptions(config.getName(), config) || optionsApplied;
    }

    return optionsApplied;
}

/**
 * The list of detector configurations is read from the configuration defined in 'detector_file'
 */
std::list<Configuration>& ConfigManager::getDetectorConfigurations() { return detector_configs_; }

/**
 * @warning A previously stored configuration is directly invalidated if the same unique name is used again
 *
 * An instance configuration is a specialized configuration for a particular module instance. If a ModuleIdentifier already
 * exists an error is thrown.
 */
Configuration& ConfigManager::addInstanceConfiguration(const ModuleIdentifier& identifier, const Configuration& config) {
    // Check uniqueness
    if(instance_identifier_to_config_.find(identifier) != instance_identifier_to_config_.end()) {
        throw ModuleIdentifierAlreadyAddedError(identifier);
    }

    // Add configuration
    instance_configs_.push_back(config);
    Configuration& ret_config = instance_configs_.back();
    instance_identifier_to_config_[identifier] = --instance_configs_.end();

    // Add identifier key to config
    ret_config.set<std::string>("identifier", identifier.getIdentifier());

    // Apply instance options
    module_option_parser_.applyOptions(identifier.getUniqueName(), ret_config);
    return ret_config;
}

/**
 * An instance configuration might be dropped when not used (e.g. it is overwritten by another module instance afterwards)
 * We need to remove it from the instance configuration list to ensure dumping the config actually dumps only the instance
 * configurations that were used.
 */
void ConfigManager::dropInstanceConfiguration(const ModuleIdentifier& identifier) {
    // Remove config from instance configs and from instance identifier map
    if(instance_identifier_to_config_.find(identifier) != instance_identifier_to_config_.end()) {
        instance_configs_.erase(instance_identifier_to_config_[identifier]);
        instance_identifier_to_config_.erase(identifier);
    } else {
        throw ModuleIdentifierNotFoundError(identifier);
    }
}
