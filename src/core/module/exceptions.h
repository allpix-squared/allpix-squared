/**
 * @file
 * Collection of all module exceptions
 *
 * @copyright MIT License
 */

#ifndef ALLPIX_MODULE_EXCEPTIONS_H
#define ALLPIX_MODULE_EXCEPTIONS_H

#include <string>

#include "core/utils/exceptions.h"
#include "core/utils/type.h"

namespace allpix {
    /**
     * @ingroup Exceptions
     * @brief Notifies of an error with dynamically loading a module
     *
     * Module library can either not be found, is outdated or invalid in general.
     */
    class DynamicLibraryError : public RuntimeError {
    public:
        /**
         * @brief Constructs error for provided module
         * @param module Name of the module that cannot be loaded
         */
        explicit DynamicLibraryError(const std::string& module) {
            error_message_ = "Dynamic library loading failed for module " + module;
        }
    };

    /**
     * @ingroup Exceptions
     * @brief Raised if ambigious instantiation of two similar modules occurs
     *
     * The framework cannot decide which module to instantiate of two with the same \ref ModuleIdentifier::getUniqueName
     * "unique name", because they have the same priority.
     */
    class AmbiguousInstantiationError : public RuntimeError {
    public:
        /**
         * @brief Constructs error with the name of the problematic module
         * @param module Name of the module that is ambigious
         */
        explicit AmbiguousInstantiationError(const std::string& module) {
            // FIXME: add detector and input output instance here
            error_message_ = "Two modules of type " + module +
                             " instantiated with same unique identifier and priority, cannot choose correct one";
        }
    };

    /**
     * @ingroup Exceptions
     * @brief Informs that a module is in a state it should never be
     *
     * The module does for example not properly forward the constructor passed in a detector module.
     */
    class InvalidModuleStateException : public LogicError {
    public:
        explicit InvalidModuleStateException(std::string message) { error_message_ = std::move(message); }
    };

    /**
     * @ingroup Exceptions
     * @brief Informs that a module executes an action is it not allowed to do in particular state
     *
     * A module for example tries to accesses special methods as Module::getOutputPath which are not allowed in the
     * constructors, or sends a message outside the Module::run method.
     */
    class InvalidModuleActionException : public LogicError {
    public:
        explicit InvalidModuleActionException(std::string message) { error_message_ = std::move(message); }
    };

    /**
     * @ingroup Exceptions
     * @brief General exception for modules if something goes wrong
     *
     * This error can be raised by modules, if a problem comes up that cannot be foreseen by the framework itself.
     */
    class ModuleError : public RuntimeError {
    public:
        explicit ModuleError(std::string reason) { error_message_ = std::move(reason); }
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_EXCEPTIONS_H */
