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
    error_message_ =
        "Value " + config.getText(key) + " of key '" + key + "' in section '" + config.getName() + "' is not valid";
    if(!reason.empty()) {
        error_message_ += ": " + reason;
    }
}
