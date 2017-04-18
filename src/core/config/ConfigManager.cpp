#include "ConfigManager.hpp"

#include <fstream>
#include <string>
#include <vector>

#include "Configuration.hpp"
#include "core/utils/file.h"
#include "exceptions.h"

using namespace allpix;

// Constructors and destructor
ConfigManager::ConfigManager(std::string file_name)
    : file_name_(std::move(file_name)), reader_(), global_default_name_(), global_names_(), ignore_names_() {
    // check if the file exists
    std::ifstream file(file_name_);
    if(!file) {
        throw allpix::ConfigFileUnavailableError(file_name_);
    }

    // convert main file to absolute path
    file_name_ = get_absolute_path(file_name_);

    // read the file
    reader_.add(file, file_name_);
}

// Reload whole config
void ConfigManager::reload() {
    reader_.clear();

    std::ifstream file(file_name_);
    reader_.add(file);
}

// Add a section name for a global header and set it to be the default name
void ConfigManager::setGlobalHeaderName(std::string name) {
    global_names_.emplace(name);
    global_default_name_ = std::move(name);
}

// Add a section name for a global header
void ConfigManager::addGlobalHeaderName(std::string name) {
    global_names_.emplace(std::move(name));
}

// Get the configuration that is global
Configuration ConfigManager::getGlobalConfiguration() {
    Configuration global_config(global_default_name_, file_name_);
    for(auto& global_name : global_names_) {
        // add all global configurations that do exists
        auto configs = getConfigurations(global_name);
        for(auto& config : configs) {
            global_config.merge(config);
        }
    }
    return global_config;
}

// Add a section name which should be ignored
void ConfigManager::addIgnoreHeaderName(std::string name) {
    ignore_names_.emplace(std::move(name));
}

// Check if configuration key exists at least once
bool ConfigManager::hasConfiguration(const std::string& name) const {
    return reader_.hasConfiguration(name);
}

// Count the amount of configurations of a given name
unsigned int ConfigManager::countConfigurations(const std::string& name) const {
    return reader_.countConfigurations(name);
}

// Return configuration by name
std::vector<Configuration> ConfigManager::getConfigurations(const std::string& name) const {
    return reader_.getConfigurations(name);
}

// return all configurations
std::vector<Configuration> ConfigManager::getConfigurations() const {
    std::vector<Configuration> result;
    for(auto& config : reader_.getConfigurations()) {
        // ignore all global and ignores names
        if(global_names_.find(config.getName()) != global_names_.end() ||
           ignore_names_.find(config.getName()) != ignore_names_.end()) {
            continue;
        }
        result.push_back(config);
    }

    return result;
}
