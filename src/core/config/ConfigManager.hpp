/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_CONFIG_MANAGER_H
#define ALLPIX_CONFIG_MANAGER_H

#include <set>
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

        // Define special sections
        void setGlobalHeaderName(std::string);
        void addGlobalHeaderName(std::string);
        Configuration getGlobalConfiguration();
        void addIgnoreHeaderName(std::string);

        // Check if configuration section exist
        bool hasConfiguration(const std::string& name) const;
        unsigned int countConfigurations(const std::string& name) const;

        // Return configuration sections by name
        std::vector<Configuration> getConfigurations(const std::string& name) const;

        // Return all configurations with their name
        std::vector<Configuration> getConfigurations() const;

    private:
        // Reader for the config files
        ConfigReader reader_;

        // List of names which indicate global sections or sections to ignore
        std::string global_default_name_;
        std::set<std::string> global_names_;
        std::set<std::string> ignore_names_;

        // File names of the added config files
        std::vector<std::string> file_names_;
    };
} // namespace allpix

#endif /* ALLPIX_CONFIG_MANAGER_H */
