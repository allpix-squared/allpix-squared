#include "SimpleConfigManager.hpp"

#include <fstream>
#include <string>
#include <vector>

#include "../utils/exceptions.h"

using namespace allpix;

// Constructors and destructor
SimpleConfigManager::SimpleConfigManager(): conf_map_(), conf_array_(), file_names_() {}
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
    build_config(file, file_name);
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
        build_config(file);
    }
}

// Clear the current config
void SimpleConfigManager::clear() {
    conf_array_.clear();
    conf_map_.clear();
}

// Check if configuration key exists at least once
bool SimpleConfigManager::hasConfiguration(std::string name) const {
    return conf_map_.find(name) != conf_map_.end();
}

// Count the amount of configurations of a given name
int SimpleConfigManager::countConfigurations(std::string name) const {
    if (!hasConfiguration(name)) {
        return 0;
    }
    return conf_map_.at(name).size();
}

// Return configuration by name
std::vector<Configuration> SimpleConfigManager::getConfigurations(std::string name) const {
    if (!hasConfiguration(name)) {
        return std::vector<Configuration>();
    }
    
    // fetch the configurations
    std::vector<Configuration> result;
    for (auto &iter : conf_map_.at(name)) {
        result.push_back(*iter);
    }
    return result;
}

// return all configurations
std::vector<Configuration> SimpleConfigManager::getConfigurations() const {
    return std::vector<Configuration>(conf_array_.begin(), conf_array_.end());;
}

// construct the config from an input stream
void SimpleConfigManager::build_config(std::istream &stream, std::string file_name) {
    std::string section_name = "";
    Configuration conf(section_name);
    
    int line_num = 0;
    while (true) {
        // process config line by line
        std::string line;
        if (stream.eof()) break;
        std::getline(stream, line);
        ++line_num;
                
        // find equal sign
        size_t equals_pos = line.find('=');
        if (equals_pos == std::string::npos) {
            line = allpix::trim(line);
            
            // ignore empty lines or comments
            if (line == "" || line[0] == ';' || line[0] == '#')
                continue;
            
            // parse new section
            if (line[0] == '[' && line[line.length() - 1] == ']') {
                // add previous section
                conf_array_.push_back(conf);
                conf_map_[section_name].push_back(--conf_array_.end());
                
                // begin new section
                section_name = std::string(line, 1, line.length() - 2);
                conf = Configuration(section_name);
            } else {
                // FIXME: should be a bit more helpful of course...
                throw ConfigParseError(file_name, line_num);
            }
        } else {
            std::string key = trim(std::string(line, 0, equals_pos));
            
            std::string value = trim(std::string(line, equals_pos + 1));
            char ins = 0;
            for(size_t i=0; i<value.size(); ++i){
                if(value[i] == '\'' || value[i] == '\"'){
                    if(!ins) ins = value[i];
                    else if(ins == value[i]) ins = 0;
                }
                if(ins == 0 && (value[i] == ';' || value[i] == '#')){
                    value = std::string(value, 0, i);
                    break;
                }
            }
                                    
            // add the config key
            conf.set(key, trim(value));
        }
    }
    // add last section
    conf_array_.push_back(conf);
    conf_map_[section_name].push_back(--conf_array_.end());
}
