/**
 * @file
 * @brief Base exceptions used in the framework
 *
 * @copyright Copyright (c) 2016-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

/**
 * @defgroup Exceptions Exception classes
 * @brief Collection of all the exceptions used in the framework
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
        explicit Exception(std::string what_arg) : error_message_(std::move(what_arg)) {}

        /**
         * @brief Return the error message
         * @return Text describing the error
         */
        const char* what() const noexcept override { return error_message_.c_str(); }

    protected:
        /**
         * @brief Internal constructor for exceptions setting the error message indirectly
         */
        Exception() = default;

        std::string error_message_; // NOLINT(misc-non-private-member-variables-in-classes)
    };

    /**
     * @ingroup Exceptions
     * @brief Errors related to problems occurring at runtime
     *
     * Problems that could never have been detected at compile time
     */
    class RuntimeError : public Exception {
    public:
        /**
         * @brief Creates exception with the given runtime problem
         * @param what_arg Text describing the problem
         */
        explicit RuntimeError(std::string what_arg) : Exception(std::move(what_arg)) {}

    protected:
        /**
         * @brief Internal constructor for exceptions setting the error message indirectly
         */
        RuntimeError() = default;
    };

    /**
     * @ingroup Exceptions
     * @brief Errors related to logical problems in the code structure
     *
     * Problems that could also have been detected at compile time by specialized software
     */
    class LogicError : public Exception {
    public:
        /**
         * @brief Creates exception with the given logical problem
         * @param what_arg Text describing the problem
         */
        explicit LogicError(std::string what_arg) : Exception(std::move(what_arg)) {}

    protected:
        /**
         * @brief Internal constructor for exceptions setting the error message indirectly
         */
        LogicError() = default;
    };
} // namespace allpix

#endif /* ALLPIX_EXCEPTIONS_H */
