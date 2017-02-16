/**
 *  @author Simon Spannagel <simon.spannagel@cern.ch>
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_CONFIGURATION_H
#define ALLPIX_CONFIGURATION_H

#include <iomanip>
#include <vector>
#include <string>
#include <map>

#include "../utils/string.h"

namespace allpix {
    
    class Configuration {
    public:
        //Constructor
        // FIXME: we need a default constructor for temporary configuration objects, do we want this...
        Configuration();
        Configuration(std::string name);
        
        // Has value
        bool has(const std::string &key) const;
        
        // Get members (FIXME: default constant?)
        template <typename T> T get(const std::string &key) const;
        template <typename T> T get(const std::string &key, const T &def) const;
        // FIXME: also specialize this for other container types (and use same structure as above)
        template <typename T> std::vector<T> getArray(const std::string &key) const;
        template <typename T> std::vector<T> getArray(const std::string &key, const std::vector<T> &def) const;
        // FIXME: better name for not parsed
        std::string getText(const std::string &key) const;
        std::string getText(const std::string &key, const std::string &def) const;
        
        // Set members
        // NOTE: see FIXME above
        template <typename T> void set(const std::string &key, const T &val);
        template <typename T> void setArray(const std::string &key, const std::vector<T> &val);
        
        // Set defaults
        // NOTE: see FIXME above
        template <typename T> void setDefault(const std::string &key, const T &val);
        template <typename T> void setDefaultArray(const std::string &key, const std::vector<T> &val);

        // FIXME: overload [] operator too
        
        // Configuration name
        std::string getName() const;
            
        // Print debug functions
        // FIXME: not very useful now
        void print(std::ostream &out) const;
        void print() const;
                                    
    private:
        using ConfigMap = std::map<std::string, std::string>;
        ConfigMap config_;
        
        std::string name_;
    };
    
    template <typename T> T Configuration::get(const std::string &key) const{
        return allpix::from_string<T>(config_.at(key));
    }
    template <typename T> T Configuration::get(const std::string &key, const T &def) const {
        if(has(key)) return get<T>(key);
        else return def;
    }
    
    template <typename T> std::vector<T> Configuration::getArray(const std::string &key) const{
        std::string str = config_.at(key);
        return allpix::split<T>(str, " ,");
    }
    template <typename T> std::vector<T> Configuration::getArray(const std::string &key, const std::vector<T> &def) const{
        if(has(key)) return getArray<T>(key);
        else return def;
    }
    
    template <typename T> void Configuration::set(const std::string &key, const T &val) {
        config_[key] = allpix::to_string(val);
    }
    template <typename T> void Configuration::setArray(const std::string &key, const std::vector<T> &val) {
        //NOTE: not the most elegant way to support arrays
        std::string str;
        for(auto &el : val){
            str += " ";
            str += val;
        }
        set(key, str);
    }
    
    template <typename T> void Configuration::setDefault(const std::string &key, const T &val) {
        if(!has(key)) set<T>(key, val);
    }
    template <typename T> void Configuration::setDefaultArray(const std::string &key, const std::vector<T> &val) {
        if(!has(key)) setArray<T>(key, val);
    }
}

#endif // ALLPIX_CONFIGURATION_H
