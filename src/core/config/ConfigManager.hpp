/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_CONFIG_MANAGER_H
#define ALLPIX_CONFIG_MANAGER_H

#include <vector>
#include <string>
#include <utility>

#include "Configuration.hpp"

namespace allpix {

    class ConfigManager {
    public:
        // Constructor and destructors
        ConfigManager() {}
        virtual ~ConfigManager() {}
        
        // Disallow copy
        ConfigManager(const ConfigManager&) = delete;
        ConfigManager &operator=(const ConfigManager&) = delete;
        
        // Check if configuration section exist
        virtual bool hasConfiguration(std::string name) const = 0;
        virtual int countConfigurations(std::string name) const = 0;
        
        // FIXME: do we want to have a special function for types where only a single instance make sense
        // Configuration getConfiguration(std::string name) = 0;
        
        // Return configuration sections by name
        virtual std::vector<Configuration> getConfigurations(std::string name) const = 0;
        
        // Return all configurations with their name
        virtual std::vector<Configuration> getConfigurations() const = 0;
    };
}

#endif /* ALLPIX_CONFIG_MANAGER_H */
