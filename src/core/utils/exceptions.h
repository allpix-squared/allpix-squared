/**
 * AllPix exception classes
 *
 * @author Simon Spannagel <simon.spannagel@cern.ch>
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_EXCEPTIONS_H
#define ALLPIX_EXCEPTIONS_H

#include <exception>
#include <string>

#include "type.h"

namespace allpix {
    // NOTE: asserts are used inside the framework for internal errors that should normally be impossible

    /**
     * Base class for all exceptions thrown by the allpix framework.
     */
    class Exception : public std::exception {
    public:
        Exception() : error_message_("Unspecified error") {}
        explicit Exception(std::string what_arg) : std::exception(), error_message_(std::move(what_arg)) {}
        const char* what() const noexcept override { return error_message_.c_str(); }

    protected:
        std::string error_message_;
    };

    /**
     * Errors related to problems at runtime
     */
    class RuntimeError : public Exception {};

    /**
     * Errors related to incorrect setup of a module (against the rules)
     */
    class LogicError : public Exception {};
}

#endif /* ALLPIX_EXCEPTIONS_H */
