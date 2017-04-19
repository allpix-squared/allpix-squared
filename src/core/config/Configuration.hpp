/**
 *  @author Simon Spannagel <simon.spannagel@cern.ch>
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_CONFIGURATION_H
#define ALLPIX_CONFIGURATION_H

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/utils/string.h"

#include "exceptions.h"

namespace allpix {

    class Configuration {
    public:
        // Constructors
        Configuration();
        explicit Configuration(std::string name);
        explicit Configuration(std::string name, std::string path);

        // Has value
        bool has(const std::string& key) const;

        // Get members
        template <typename T> T get(const std::string& key) const;
        template <typename T> T get(const std::string& key, const T& def) const;

        // FIXME: also specialize this for other container types (and use same structure as above)
        template <typename T> std::vector<T> getArray(const std::string& key) const;

        // Get as literal text (without conversion)
        std::string getText(const std::string& key) const;
        std::string getText(const std::string& key, const std::string& def) const;

        // Get paths with relative converted to absolutes
        std::string getPath(const std::string& key, bool check_exists = false) const;
        std::vector<std::string> getPathArray(const std::string& key, bool check_exists = false) const;

        // Set members (NOTE: not literally)
        // NOTE: see FIXME above
        template <typename T> void set(const std::string& key, const T& val);
        template <typename T> void setArray(const std::string& key, const std::vector<T>& val);

        // Set defaults
        // NOTE: see FIXME above
        template <typename T> void setDefault(const std::string& key, const T& val);
        template <typename T> void setDefaultArray(const std::string& key, const std::vector<T>& val);

        // Set literal text
        void setText(const std::string& key, const std::string& val);

        // FIXME: overload [] operator too

        // Count amount of values
        unsigned int countSettings() const;

        // Configuration name and path
        std::string getName() const;
        std::string getFilePath() const; // FIXME: name clash with getPath

        // Merge other configuration
        void merge(const Configuration& other);

        // Print debug functions
        // FIXME: not very useful now
        void print(std::ostream& out) const;

    private:
        // convert path to absolute path if necessary
        std::string path_to_absolute(std::string path, bool normalize_path) const;

        // name of the section
        std::string name_;
        // path where this section resides
        std::string path_;

        // actual configuration storage
        using ConfigMap = std::map<std::string, std::string>;
        ConfigMap config_;
    };

    // Get parameter
    template <typename T> T Configuration::get(const std::string& key) const {
        try {
            return allpix::from_string<T>(config_.at(key));
        } catch(std::out_of_range& e) {
            throw MissingKeyError(key, getName());
        } catch(std::invalid_argument& e) {
            throw InvalidKeyError(key, getName(), config_.at(key), typeid(T), e.what());
        } catch(std::overflow_error& e) {
            throw InvalidKeyError(key, getName(), config_.at(key), typeid(T), e.what());
        }
    }
    // Get parameter with default
    template <typename T> T Configuration::get(const std::string& key, const T& def) const {
        if(has(key)) {
            return get<T>(key);
        }
        return def;
    }

    // Get array of parameter types
    template <typename T> std::vector<T> Configuration::getArray(const std::string& key) const {
        try {
            std::string str = config_.at(key);
            return allpix::split<T>(str, " ,");
        } catch(std::out_of_range& e) {
            throw MissingKeyError(key, getName());
        } catch(std::invalid_argument& e) {
            throw InvalidKeyError(key, getName(), config_.at(key), typeid(T), e.what());
        } catch(std::overflow_error& e) {
            throw InvalidKeyError(key, getName(), config_.at(key), typeid(T), e.what());
        }
    }

    // Set converted value
    template <typename T> void Configuration::set(const std::string& key, const T& val) {
        config_[key] = allpix::to_string(val);
    }

    // Set array of values
    template <typename T> void Configuration::setArray(const std::string& key, const std::vector<T>& val) {
        // NOTE: not the most elegant way to support arrays
        std::string str;
        for(auto& el : val) {
            str += " ";
            str += allpix::to_string(val);
        }
        set(key, str);
    }

    // Set default values (sets if the key does not exists)
    template <typename T> void Configuration::setDefault(const std::string& key, const T& val) {
        if(!has(key)) {
            set<T>(key, val);
        }
    }

    // Set default array values (sets if the key does not exists)
    template <typename T> void Configuration::setDefaultArray(const std::string& key, const std::vector<T>& val) {
        if(!has(key)) {
            setArray<T>(key, val);
        }
    }
} // namespace allpix

#endif /* ALLPIX_CONFIGURATION_H */
