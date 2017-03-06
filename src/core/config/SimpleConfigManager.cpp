#include "SimpleConfigManager.hpp"

#include <fstream>
#include <string>
#include <vector>

#include "../utils/exceptions.h"

using namespace allpix;

// Constructors and destructor
SimpleConfigManager::SimpleConfigManager(): reader_(), file_names_() {}
SimpleConfigManager::SimpleConfigManager(std::string file_name): SimpleConfigManager() {
    addFile(file_name);
}
SimpleConfigManager::~SimpleConfigManager() {}

// Add a new settings file
void SimpleConfigManager::addFile(std::string file_name) {
    file_names_.push_back(file_name);
    std::ifstream file(file_name);
    if (!file) {
        throw allpix::ConfigFileUnavailableError(file_name);
    }
    reader_.add(file, file_name);
}

// Remove all files and clear the config
void SimpleConfigManager::removeFiles() {
    file_names_.clear();
    clear();
}

// Reload all files
void SimpleConfigManager::reload() {
    clear();
    
    for (auto &file_name : file_names_) {
        std::ifstream file(file_name);
        reader_.add(file);
    }
}

// Clear the current config
void SimpleConfigManager::clear() {
    reader_.clear();
}

// Check if configuration key exists at least once
bool SimpleConfigManager::hasConfiguration(std::string name) const {
    return reader_.hasConfiguration(name);
}

// Count the amount of configurations of a given name
int SimpleConfigManager::countConfigurations(std::string name) const {
    return reader_.hasConfiguration(name);
}

// Return configuration by name
std::vector<Configuration> SimpleConfigManager::getConfigurations(std::string name) const {
    return reader_.getConfigurations(name);
}

// return all configurations
std::vector<Configuration> SimpleConfigManager::getConfigurations() const {
    return reader_.getConfigurations();
}
