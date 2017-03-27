/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_INVALID_VALUE_ERROR_H
#define ALLPIX_INVALID_VALUE_ERROR_H

#include "Configuration.hpp"
#include "exceptions.h"

namespace allpix {
    // Value is not valid data for the module (but type is correct so can only detected by modules)
    class InvalidValueError : public ConfigurationError {
    public:
        InvalidValueError(Configuration config, const std::string& key, const std::string& reason) {
            error_message_ = "Value '" + config.getText(key) + "' of key '" + key + "' in section '" + config.getName() +
                             "' is not valid";
            if(!reason.empty()) {
                error_message_ += ": " + reason;
            }
        }
        InvalidValueError(Configuration config, const std::string& key) : InvalidValueError(config, key, "") {}
    };
}

#endif /* ALLPIX_INVALID_VALUE_ERROR_H */
