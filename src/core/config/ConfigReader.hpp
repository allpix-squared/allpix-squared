/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_CONFIG_READER_H
#define ALLPIX_CONFIG_READER_H

#include <istream>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "Configuration.hpp"

namespace allpix {

    class ConfigReader {
    public:
        // Constructor and destructors
        ConfigReader();
        explicit ConfigReader(std::istream&);
        ConfigReader(std::istream&, std::string);

        // Add stream
        void add(std::istream&);
        void add(std::istream&, std::string file_name);

        // Clear whole config
        void clear();

        // Check if configuration section exist
        bool hasConfiguration(const std::string& name) const;
        unsigned int countConfigurations(const std::string& name) const;

        // Return configuration sections by name
        std::vector<Configuration> getConfigurations(const std::string& name) const;

        // Return all configurations with their name
        std::vector<Configuration> getConfigurations() const;

    private:
        std::map<std::string, std::vector<std::list<Configuration>::iterator>> conf_map_;
        std::list<Configuration> conf_array_;
    };
} // namespace allpix

#endif /* ALLPIX_CONFIG_MANAGER_H */
