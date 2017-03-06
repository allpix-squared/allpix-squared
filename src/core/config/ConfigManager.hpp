/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_CONFIG_MANAGER_H
#define ALLPIX_CONFIG_MANAGER_H

#include <vector>
#include <list>
#include <string>
#include <utility>
#include <fstream>
#include <map>

#include "Configuration.hpp"

#include "ConfigReader.hpp"

namespace allpix {

    class ConfigManager{
    public:
        // Constructor and destructors
        ConfigManager();
        explicit ConfigManager(std::string file_name);
        ~ConfigManager();
        
        // Disallow copy
        ConfigManager(const ConfigManager&) = delete;
        ConfigManager &operator=(const ConfigManager&) = delete;
        
        // Add file
        void addFile(std::string file_name);
        void removeFiles();
        
        // Reload all configs (clears and rereads)
        void reload();
        
        // Clear all configuration
        void clear();
        
        // Check if configuration section exist
        bool hasConfiguration(std::string name) const;
        int countConfigurations(std::string name) const;
        
        // Return configuration sections by name
        std::vector<Configuration> getConfigurations(std::string name) const;
        
        // Return all configurations with their name
        std::vector<Configuration> getConfigurations() const;
        
    private:
        ConfigReader reader_;
        
        std::vector<std::string> file_names_;
    };
}

#endif /* ALLPIX_CONFIG_MANAGER_H */
