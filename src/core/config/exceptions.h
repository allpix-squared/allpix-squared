/**
 * AllPix config exception classes
 *
 * @author Simon Spannagel <simon.spannagel@cern.ch>
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_CONFIG_EXCEPTIONS_H
#define ALLPIX_CONFIG_EXCEPTIONS_H

#include <string>

#include "core/utils/exceptions.h"
#include "core/utils/type.h"

namespace allpix {
    /*
     * Base class for all configuration related errors
     */
    class ConfigurationError : public Exception {};

    /*
     * Error if a config file could not be read
     */
    class ConfigFileUnavailableError : public ConfigurationError {
    public:
        explicit ConfigFileUnavailableError(const std::string& file) {
            error_message_ = "Could not read configuration file " + file + " (does it exists?)";
        }
    };

    // invalid key in the configuration
    // FIXME: this should probably be InvalidValueTypeError (see below)
    class InvalidKeyError : public ConfigurationError {
    public:
        InvalidKeyError(const std::string& key,
                        const std::string& section,
                        const std::string& value,
                        const std::type_info& type,
                        const std::string& reason) {
            error_message_ = "Could not convert value '" + value + "' of key '" + key + "' in section '" + section +
                             "' to type " + allpix::demangle(type.name());
            if(!reason.empty()) {
                error_message_ += ": " + reason;
            }
        }
        InvalidKeyError(const std::string& key,
                        const std::string& section,
                        const std::string& value,
                        const std::type_info& type)
            : InvalidKeyError(key, section, value, type, "") {}
    };

    // missing key in the configuration
    class MissingKeyError : public ConfigurationError {
    public:
        MissingKeyError(const std::string& key, const std::string& section) {
            error_message_ = "Key '" + key + "' in section '" + section + "' does not exist";
        }
    };

    // parse error in the configuration
    class ConfigParseError : public ConfigurationError {
    public:
        ConfigParseError(const std::string& file, int line_num) {
            error_message_ = "Could not parse line ";
            error_message_ += std::to_string(line_num);
            error_message_ += " in file '" + file + "'";
            error_message_ += ": not a section header, key/value pair or comment";
        }
    };
}

#endif /* ALLPIX_CONFIG_EXCEPTIONS_H */
