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
        Configuration(std::string name);
        
        // Get members
        //std::string get(const std::string &key, const std::string &def) const;
        //double get(const std::string &key, double def) const;
        //int64_t get(const std::string &key, int64_t def) const;
        //uint64_t get(const std::string &key, uint64_t def) const;
        //int get(const std::string &key, int def) const;
        template <typename T> T get(const std::string &key, T def) const {
            return allpix::from_string(Get(key, allpix::to_string(def)), def);
        }
        template <typename T> T get(const std::string &key, const std::string fallback,
            const T &def) const {
            return get(key, get(fallback, def));
        }
        /*template <typename T> std::vector<T> getArray(const std::string &key, std::vector<T> def) const {
            *      return split(Get(key,std::string()),def,',');
        }*/
        
        // Set members
        template <typename T> void set(const std::string &key, const T &val);

        // Configuration name
        std::string getName() const;
            
        // Print debug functions
        // FIXME: not very useful now
        void print(std::ostream &out) const;
        void print() const;
                                    
    private:
        std::string get_string(const std::string &key) const;
        void set_string(const std::string &key, const std::string &val);
        
        typedef std::map<std::string, std::string> config_t;
        config_t config_;
        
        std::string name_;
    };
}

#endif // ALLPIX_CONFIGURATION_H
