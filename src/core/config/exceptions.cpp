/**
 * @file
 * @brief Implementation of configuration exceptions
 *
 * @copyright MIT License
 */

#include "exceptions.h"
#include "Configuration.hpp"

using namespace allpix;

InvalidValueError::InvalidValueError(const Configuration& config, const std::string& key, const std::string& reason) {
    std::string section_str = "in section '" + config.getName() + "'";
    if(config.getName().empty()) {
        section_str = "in empty section";
    }
    error_message_ = "Value " + config.getText(key) + " of key '" + key + "' " + section_str + " is not valid";
    if(!reason.empty()) {
        error_message_ += ": " + reason;
    }
}
