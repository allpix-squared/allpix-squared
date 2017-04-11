#include "ConfigManager.hpp"

#include <fstream>
#include <string>
#include <vector>

#include "Configuration.hpp"
#include "exceptions.h"

using namespace allpix;

// Constructors and destructor
ConfigManager::ConfigManager() : reader_(), file_names_() {}
ConfigManager::ConfigManager(std::string file_name) : ConfigManager() {
    addFile(file_name);
}
ConfigManager::~ConfigManager() = default;

// Add a new settings file
void ConfigManager::addFile(const std::string& file_name) {
    file_names_.push_back(file_name);
    std::ifstream file(file_name);
    if(!file) {
        throw allpix::ConfigFileUnavailableError(file_name);
    }
    reader_.add(file, file_name);
}

// Remove all files and clear the config
void ConfigManager::removeFiles() {
    file_names_.clear();
    clear();
}

// Reload all files
void ConfigManager::reload() {
    clear();

    for(auto& file_name : file_names_) {
        std::ifstream file(file_name);
        reader_.add(file);
    }
}

// Clear the current config
void ConfigManager::clear() {
    reader_.clear();
}

// Add a section name for a global header
void ConfigManager::addGlobalHeaderName(std::string name) {
    global_names_.emplace(std::move(name));
}

// Get the configuration that is global
Configuration ConfigManager::getGlobalConfiguration() {
    Configuration global_config;
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
