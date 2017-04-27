#include "ConfigReader.hpp"

#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include "core/utils/file.h"
#include "exceptions.h"

using namespace allpix;

// Constructors and destructor
ConfigReader::ConfigReader() : conf_map_(), conf_array_() {}
ConfigReader::ConfigReader(std::istream& stream) : ConfigReader(stream, "") {}
ConfigReader::ConfigReader(std::istream& stream, std::string file_name) : ConfigReader() {
    add(stream, std::move(file_name));
}

// Add stream and construct the config
void ConfigReader::add(std::istream& stream) {
    add(stream, "");
}
void ConfigReader::add(std::istream& stream, std::string file_name) {
    // convert file name to absolute path (if given)
    if(!file_name.empty()) {
        file_name = allpix::get_absolute_path(file_name);
    }

    // build first empty configuration
    std::string section_name;
    Configuration conf(section_name, file_name);

    int line_num = 0;
    while(true) {
        // process config line by line
        std::string line;
        if(stream.eof()) {
            break;
        }
        std::getline(stream, line);
        ++line_num;

        // find equal sign
        size_t equals_pos = line.find('=');
        if(equals_pos == std::string::npos) {
            line = allpix::trim(line);

            // ignore empty lines or comments
            if(line == "" || line[0] == '#') {
                continue;
            }

            // parse new section
            if(line[0] == '[' && line[line.length() - 1] == ']') {
                // ignore empty sections if they contain no configurations
                if(!conf.getName().empty() || conf.countSettings() > 0) {
                    // add previous section
                    conf_array_.push_back(conf);
                    conf_map_[section_name].push_back(--conf_array_.end());
                }

                // begin new section
                section_name = std::string(line, 1, line.length() - 2);
                conf = Configuration(section_name, file_name);
            } else {
                // FIXME: should be a bit more helpful...
                throw ConfigParseError(file_name, line_num);
            }
        } else {
            std::string key = trim(std::string(line, 0, equals_pos));

            std::string value = trim(std::string(line, equals_pos + 1));
            char ins = 0;
            for(size_t i = 0; i < value.size(); ++i) {
                if(value[i] == '\'' || value[i] == '\"') {
                    if(ins == 0) {
                        ins = value[i];
                    } else if(ins == value[i]) {
                        ins = 0;
                    }
                }
                if(ins == 0 && value[i] == '#') {
                    value = std::string(value, 0, i);
                    break;
                }
            }

            // add the config key
            conf.setText(key, trim(value));
        }
    }
    // add last section
    conf_array_.push_back(conf);
    conf_map_[section_name].push_back(--conf_array_.end());
}

// Clear whole config
void ConfigReader::clear() {
    conf_map_.clear();
    conf_array_.clear();
}

// Check if configuration key exists at least once
bool ConfigReader::hasConfiguration(const std::string& name) const {
    return conf_map_.find(name) != conf_map_.end();
}

// Count the amount of configurations of a given name
unsigned int ConfigReader::countConfigurations(const std::string& name) const {
    if(!hasConfiguration(name)) {
        return 0;
    }
    return static_cast<unsigned int>(conf_map_.at(name).size());
}

// Return configuration by name
std::vector<Configuration> ConfigReader::getConfigurations(const std::string& name) const {
    if(!hasConfiguration(name)) {
        return std::vector<Configuration>();
    }

    // fetch the configurations
    std::vector<Configuration> result;
    for(auto& iter : conf_map_.at(name)) {
        result.push_back(*iter);
    }
    return result;
}

// Return all configurations
std::vector<Configuration> ConfigReader::getConfigurations() const {
    return std::vector<Configuration>(conf_array_.begin(), conf_array_.end());
}
