#include "SimpleConfigManager.hpp"

#include <fstream>

#include "../utils/exceptions.h"

using namespace allpix;

// constructor to add file
SimpleConfigManager::SimpleConfigManager(std::string file_name) {
    addFile(file_name);
}

// add a new settings file
void SimpleConfigManager::addFile(std::string file_name) {
    file_names_.push_back(file_name);
    std::ifstream file(file_name);
    if(!file){
        throw allpix::exception("Config file cannot be read");
    }
    build_config(file);
}

// remove all files and clear the config
void SimpleConfigManager::removeFiles() {
    file_names_.clear();
    clear();
}

// reload all files
void SimpleConfigManager::reload() {
    clear();
    
    for(auto &file_name : file_names_){
        std::ifstream file(file_name);
        build_config(file);
    }
}

// clear the current config
void SimpleConfigManager::clear() {
    conf_map_.clear();
}

// check if configuration key exists at least once
bool SimpleConfigManager::hasConfiguration(std::string name) const {
    return conf_map_.find(name) != conf_map_.end();
}

// count the amount of configurations of a given name
int SimpleConfigManager::countConfigurations(std::string name) const {
    if(!hasConfiguration(name)) return 0;
    else return conf_map_.at(name).size();
}

// return configuration by name
std::vector<Configuration> SimpleConfigManager::getConfigurations(std::string name) const {
    if(!hasConfiguration(name)) return std::vector<Configuration>();
    return conf_map_.at(name);
}

// return all configurations
std::vector<Configuration> SimpleConfigManager::getConfigurations() const {
    std::vector<Configuration> result;
    
    for(auto &configs : conf_map_) {
        for(auto &conf : configs.second ) {
            result.push_back(conf);
        }
    }
    
    return result;
}

// construct the config from an input stream
void SimpleConfigManager::build_config(std::istream &stream) {
    std::string section_name = "";
    Configuration conf(section_name);
    
    while(true) {
        // process config line by line
        std::string line;
        if (stream.eof()) break;
        std::getline(stream, line);
                
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
                conf_map_[section_name].push_back(conf);
                
                // begin new section
                section_name = std::string(line, 1, line.length() - 2);
                conf = Configuration(section_name);
            } else {
                // FIXME: should be a bit more helpful of course...
                throw allpix::exception("Parse error: not a section header, comment or value");
            }
        } else {
            std::string key = trim(std::string(line, 0, equals_pos));
            // FIXME: handle lines like: blah = "foo said ""bar""; # ok." # not "baz"
            // FIXME: this will break for arrays like "foor bar" "baz" which should parse as ["foo bar", "baz"] of course
            std::string value = trim(std::string(line, equals_pos + 1));
            if ((line[0] == '\'' && line[line.length() - 1] == '\'') ||
                (line[0] == '\"' && line[line.length() - 1] == '\"')) {
                line = std::string(line, 1, line.length() - 2);
            } else {
                size_t i = value.find_first_of(";#");
                if (i != std::string::npos)
                    value = trim(std::string(value, 0, i));
            }
                        
            // add the config key
            conf.set(key, value);
        }
    }
    // add last section
    conf_map_[section_name].push_back(conf);
}
