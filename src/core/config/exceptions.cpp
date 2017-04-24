/**
 * AllPix config exception classes
 *
 * @author Koen Wolters <koen.wolters@cern.ch>
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
InvalidValueError::InvalidValueError(const Configuration& config, const std::string& key)
    : InvalidValueError(config, key, "") {}
