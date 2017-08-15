/**
 * @file
 * @brief Implementation of config manager
 * @copyright MIT License
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
    Configuration global_config(global_default_name_, file_name_);
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
