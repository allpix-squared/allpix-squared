/**
 * AllPix exception classes
 *
 * @author Simon Spannagel <simon.spannagel@cern.ch>
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_EXCEPTIONS_H
#define ALLPIX_EXCEPTIONS_H

#include <exception>
#include <iostream>
#include <string>
#include <typeinfo>

#include "type.h"

// FIXME: move some exceptions to a more appropriate place

namespace allpix {
    // NOTE: assert are used inside the framework if an internal error should never occur

    // class Configuration;

    /**
     * Base class for all exceptions thrown by the allpix framework.
     */
    class Exception : public std::exception {
    public:
        Exception() : error_message_("Unspecified error") {}
        explicit Exception(std::string what_arg) : std::exception(), error_message_(std::move(what_arg)) {}
        const char*                    what() const noexcept override { return error_message_.c_str(); }

    protected:
        std::string error_message_;
    };

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
        InvalidKeyError(const std::string&    key,
                        const std::string&    section,
                        const std::string&    value,
                        const std::type_info& type,
                        const std::string&    reason) {
            error_message_ = "Could not convert value '" + value + "' of key '" + key + "' in section '" + section +
                             "' to type " + allpix::demangle(type.name());
            if(!reason.empty()) {
                error_message_ += ": " + reason;
            }
        }
        InvalidKeyError(const std::string&    key,
                        const std::string&    section,
                        const std::string&    value,
                        const std::type_info& type)
            : InvalidKeyError(key, section, value, type, "") {}
    };
    // value is not valid (type is correct but data not)
    // FIXME: only configuration exception not thrown by the framework but by the user and the interface is not good
    class InvalidValueError : public ConfigurationError {
    public:
        InvalidValueError(const std::string& key,
                          const std::string& section,
                          const std::string& value,
                          const std::string& reason) {
            error_message_ = "Value '" + value + "' of key '" + key + "' in section '" + section + "' is not valid";
            if(!reason.empty()) {
                error_message_ += ": " + reason;
            }
        }
        InvalidValueError(const std::string& key, const std::string& section, const std::string& value)
            : InvalidValueError(key, section, value, "") {}
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

    /*
     * Errors related to problems at runtime
     */
    class RuntimeError : public Exception {};

    /*
     * Errors related to instantiation of modules
     * FIXME: possibly an inheritance level can be added here
     *
     * NOTE: never thrown by the module itself, only by the core or the factories
     */
    class InstantiationError : public RuntimeError {
    public:
        explicit InstantiationError(const std::string& module) {
            // FIXME: add detectory and input output instance here
            error_message_ = "Could not instantiate a module of type " + module;
        }
    };
    class AmbiguousInstantiationError : public RuntimeError {
    public:
        explicit AmbiguousInstantiationError(const std::string& module) {
            // FIXME: add detectory and input output instance here
            error_message_ = "Two modules of type " + module +
                             " instantiated with same unique identifier and priority, cannot choose correct one";
        }
    };

    /*
     * Errors related to module unexpected finalization before a module has run
     * WARNING: can both be a config or logic error
     */
    class UnexpectedFinalizeException : public RuntimeError {
    public:
        explicit UnexpectedFinalizeException(const std::string& module) {
            // FIXME: add detectory and input output instance here
            error_message_ =
                "Module of type " + module + " reached finalization unexpectedly (are all required messages sent?)";
        }
    };

    /*
     * Receive of a message that a module did not expect (only one message) or if a message is sent out without receivers
     * (NOTE: not always a problem)
     */
    class UnexpectedMessageException : public RuntimeError {
    public:
        UnexpectedMessageException(const std::string& module, const std::type_info& message) {
            // FIXME: add detectory and input output instance here
            error_message_ = "Unexpected receive of message ";
            error_message_ += allpix::demangle(message.name());
            error_message_ += " by module " + module + " (are multiple modules providing the same output?)";
        }
    };

    /*
     * Errors related to detectors that do not exist
     *
     * FIXME: specialize and generalize these errors?
     */
    class InvalidDetectorError : public RuntimeError {
    public:
        InvalidDetectorError(const std::string& category, const std::string& detector) {
            error_message_ = "Could not find a detector with " + category + " '" + detector + "'";
        }
    };

    /*
     * Errors related to incorrect setup of a module (against the rules)
     */
    class LogicError : public Exception {};

    // FIXME: implement this later (possibly both can be merged into one error)

    // detect if module is in a wrong state (for example dispatching a message outside the run method run by the module
    // manager)
    class InvalidModuleStateException : public LogicError {};
    class InvalidModuleActionException : public LogicError {};

    /*
     * General exceptions for modules if something goes wrong
     * FIXME: extend this and move to module
     */
    class ModuleException : public RuntimeError {
    public:
        explicit ModuleException(std::string what_arg) { error_message_ = std::move(what_arg); }
    };
}

#endif /* ALLPIX_EXCEPTIONS_H */
