/**
 *  @author Simon Spannagel <simon.spannagel@cern.ch>
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "Configuration.hpp"

#include <ostream>
#include <string>

#include "core/utils/exceptions.h"

using namespace allpix;

Configuration::Configuration() : Configuration("") {}

Configuration::Configuration(std::string name) : name_(std::move(name)), config_() {}

bool Configuration::has(const std::string& key) const { return config_.find(key) != config_.cend(); }

std::string Configuration::getName() const { return name_; }

std::string Configuration::getText(const std::string& key) const {
    try {
        return config_.at(key);
    } catch(std::out_of_range& e) {
        throw MissingKeyError(getName(), key);
    }
}
std::string Configuration::getText(const std::string& key, const std::string& def) const {
    if(!has(key)) {
        return def;
    }
    return getText(key);
}

void Configuration::print(std::ostream& out) const {
    for(auto& element : config_) {
        out << element.first << " : " << element.second << std::endl;
    }
}
