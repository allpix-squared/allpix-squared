/**
 * @file
 * @brief Implementation of config reader
 * @copyright MIT License
 */

#include "ConfigReader.hpp"

#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include "core/utils/file.h"
#include "core/utils/log.h"
#include "exceptions.h"

using namespace allpix;

ConfigReader::ConfigReader() = default;
ConfigReader::ConfigReader(std::istream& stream, std::string file_name) : ConfigReader() {
    add(stream, std::move(file_name));
}

ConfigReader::ConfigReader(const ConfigReader& other) : conf_array_(other.conf_array_) {
    copy_init_map();
}
ConfigReader& ConfigReader::operator=(const ConfigReader& other) {
    conf_array_ = other.conf_array_;
    copy_init_map();
    return *this;
}

void ConfigReader::copy_init_map() {
    conf_map_.clear();
    for(auto iter = conf_array_.begin(); iter != conf_array_.end(); ++iter) {
        conf_map_[iter->getName()].push_back(iter);
    }
}

/**
 * @throws ConfigParseError If an error occurred during the parsing of the stream
 *
 * The configuration is immediately parsed and all of its configurations are available after the functions returns.
 */
void ConfigReader::add(std::istream& stream, std::string file_name) {
    LOG(TRACE) << "Parsing configuration file " << file_name;

    // Convert file name to absolute path (if given)
    if(!file_name.empty()) {
        file_name = allpix::get_absolute_path(file_name);
    }

    // Build first empty configuration
    std::string section_name;
    Configuration conf(section_name, file_name);

    int line_num = 0;
    while(true) {
        // Process config line by line
        std::string line;
        if(stream.eof()) {
            break;
        }
        std::getline(stream, line);
        ++line_num;

        // Find equal sign
        size_t equals_pos = line.find('=');
        if(equals_pos == std::string::npos) {
            line = allpix::trim(line);

            // Ignore empty lines or comments
            if(line == "" || line[0] == '#') {
                continue;
            }

            // Parse new section
            if(line[0] == '[' && line[line.length() - 1] == ']') {
                // Ignore empty sections if they contain no configurations
                if(!conf.getName().empty() || conf.countSettings() > 0) {
                    // Add previous section
                    conf_array_.push_back(conf);
                    conf_map_[section_name].push_back(--conf_array_.end());
                }

                // Begin new section
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

            // Add the config key
            conf.setText(key, trim(value));
        }
    }
    // Add last section
    conf_array_.push_back(conf);
    conf_map_[section_name].push_back(--conf_array_.end());
}

void ConfigReader::clear() {
    conf_map_.clear();
    conf_array_.clear();
}

bool ConfigReader::hasConfiguration(const std::string& name) const {
    return conf_map_.find(name) != conf_map_.end();
}

unsigned int ConfigReader::countConfigurations(const std::string& name) const {
    if(!hasConfiguration(name)) {
        return 0;
    }
    return static_cast<unsigned int>(conf_map_.at(name).size());
}

/**
 * @warning This will have the file path of the first header section
 * @note An empty configuration is returned if no empty section is found
 */
Configuration ConfigReader::getHeaderConfiguration() const {
    // Get empty configurations
    std::vector<Configuration> configurations = getConfigurations("");
    if(configurations.empty()) {
        // Use all configurations to find the file name if no empty
        configurations = getConfigurations();
        std::string file_name;
        if(!configurations.empty()) {
            file_name = configurations.at(0).getFilePath();
        }
        return Configuration("", file_name);
    }

    // Merge all configurations
    Configuration header_config = configurations.at(0);
    for(auto& config : configurations) {
        // NOTE: Merging first configuration again has no effect
        header_config.merge(config);
    }
    return header_config;
}

std::vector<Configuration> ConfigReader::getConfigurations(const std::string& name) const {
    if(!hasConfiguration(name)) {
        return std::vector<Configuration>();
    }

    std::vector<Configuration> result;
    for(auto& iter : conf_map_.at(name)) {
        result.push_back(*iter);
    }
    return result;
}

std::vector<Configuration> ConfigReader::getConfigurations() const {
    return std::vector<Configuration>(conf_array_.begin(), conf_array_.end());
}
