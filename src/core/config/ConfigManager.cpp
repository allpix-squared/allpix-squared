#include "ConfigManager.hpp"

#include <fstream>
#include <string>
#include <vector>

#include "../utils/exceptions.h"

using namespace allpix;

// Constructors and destructor
ConfigManager::ConfigManager(): reader_(), file_names_() {}
ConfigManager::ConfigManager(std::string file_name): ConfigManager() {
    addFile(file_name);
}
ConfigManager::~ConfigManager() {}

// Add a new settings file
void ConfigManager::addFile(std::string file_name) {
    file_names_.push_back(file_name);
    std::ifstream file(file_name);
    if (!file) {
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
    
    for (auto &file_name : file_names_) {
        std::ifstream file(file_name);
        reader_.add(file);
    }
}

// Clear the current config
void ConfigManager::clear() {
    reader_.clear();
}

// Check if configuration key exists at least once
bool ConfigManager::hasConfiguration(std::string name) const {
    return reader_.hasConfiguration(name);
}

// Count the amount of configurations of a given name
int ConfigManager::countConfigurations(std::string name) const {
    return reader_.hasConfiguration(name);
}

// Return configuration by name
std::vector<Configuration> ConfigManager::getConfigurations(std::string name) const {
    return reader_.getConfigurations(name);
}

// return all configurations
std::vector<Configuration> ConfigManager::getConfigurations() const {
    return reader_.getConfigurations();
}
