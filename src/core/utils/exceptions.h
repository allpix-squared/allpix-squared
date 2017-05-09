/**
 * @file
 * @brief Overall list of base exceptions used in the framework
 *
 * @defgroup Exceptions Exception classes
 */

#ifndef ALLPIX_EXCEPTIONS_H
#define ALLPIX_EXCEPTIONS_H

#include <exception>
#include <string>

#include "type.h"

namespace allpix {
    // NOTE: asserts are used inside the framework for internal errors that should be impossible for modules

    /**
     * @ingroup Exceptions
     * @brief Base class for all non-internal exceptions in framework.
     */
    class Exception : public std::exception {
    public:
        /**
         * @brief Creates exception with the specified problem
         * @param what_arg Text describing the problem
         */
        explicit Exception(std::string what_arg) : std::exception(), error_message_(std::move(what_arg)) {}

        /**
         * @brief Return the error message
         * @return Text describing the error
         */
        const char* what() const noexcept override { return error_message_.c_str(); }

    protected:
        /**
         * @brief Internal constructor for exceptions setting the error message indirectly
         */
        Exception() : error_message_() {}

        std::string error_message_;
    };

    /**
     * @ingroup Exceptions
     * @brief Errors related to problems ocurring at runtime
     *
     * Problems that could never have been detected at compile time
     */
    class RuntimeError : public Exception {};

    /**
     * @ingroup Exceptions
     * @brief Errors related to logical problems in the code structure
     *
     * Problems that could also have been detected at compile time by specialized software
     */
    class LogicError : public Exception {};
} // namespace allpix

#endif /* ALLPIX_EXCEPTIONS_H */
