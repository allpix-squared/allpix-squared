/**
 *  AllPix module exception classes
 *
 *  @author Simon Spannagel <simon.spannagel@cern.ch>
 *  @author Koen Wolters <koen.wolters@cern.ch>
 *  @author Daniel Hynds <daniel.hynds@cern.ch>
 */

#ifndef ALLPIX_MODULE_EXCEPTIONS_H
#define ALLPIX_MODULE_EXCEPTIONS_H

#include <string>

#include "core/utils/exceptions.h"
#include "core/utils/type.h"

namespace allpix {
    /*
     * Errors related to instantiation of modules
     */
    class DynamicLibraryError : public RuntimeError {
    public:
        explicit DynamicLibraryError(const std::string& module) {
            error_message_ = "Dynamic library loading failed for module " + module;
        }
    };

    class InstantiationError : public RuntimeError {
    public:
        explicit InstantiationError(const std::string& module) {
            // FIXME: add detector and input output instance here
            error_message_ = "Could not instantiate a module of type " + module;
        }
    };
    class AmbiguousInstantiationError : public RuntimeError {
    public:
        explicit AmbiguousInstantiationError(const std::string& module) {
            // FIXME: add detector and input output instance here
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
            // FIXME: add detector and input output instance here
            error_message_ =
                "Module of type " + module + " reached finalization unexpectedly (are all required messages sent?)";
        }
    };

    // detect if module is in a wrong state
    // (for example dispatching a message outside the run method run by the module manager)
    class InvalidModuleStateException : public LogicError {
    public:
        explicit InvalidModuleStateException(std::string message) { error_message_ = std::move(message); }
    };
    class InvalidModuleActionException : public LogicError {
    public:
        explicit InvalidModuleActionException(std::string message) { error_message_ = std::move(message); }
    };

    /*
     * General exceptions for modules if something goes wrong (called by modules)
     */
    class ModuleError : public RuntimeError {
    public:
        explicit ModuleError(std::string reason) { error_message_ = std::move(reason); }
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_EXCEPTIONS_H */
