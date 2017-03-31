/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_CONFIG_MANAGER_H
#define ALLPIX_CONFIG_MANAGER_H

#include <string>
#include <vector>

#include "ConfigReader.hpp"
#include "Configuration.hpp"

namespace allpix {

    class ConfigManager {
    public:
        // Constructor and destructors
        ConfigManager();
        explicit ConfigManager(std::string file_name);
        ~ConfigManager();

        // Disallow copy
        ConfigManager(const ConfigManager&) = delete;
        ConfigManager& operator=(const ConfigManager&) = delete;

        // Add file
        void addFile(const std::string& file_name);
        void removeFiles();

        // Reload all configs (clears and rereads)
        void reload();

        // Clear all configuration
        void clear();

        // Check if configuration section exist
        bool hasConfiguration(const std::string& name) const;
        unsigned int countConfigurations(const std::string& name) const;

        // Return configuration sections by name
        std::vector<Configuration> getConfigurations(const std::string& name) const;

        // Return all configurations with their name
        std::vector<Configuration> getConfigurations() const;

    private:
        ConfigReader reader_;

        std::vector<std::string> file_names_;
    };
} // namespace allpix

#endif /* ALLPIX_CONFIG_MANAGER_H */
