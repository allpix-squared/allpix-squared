/**
 * @file
 * @brief Collection of all message exceptions
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MESSENGER_EXCEPTIONS_H
#define ALLPIX_MESSENGER_EXCEPTIONS_H

#include <string>

#include "core/utils/exceptions.h"
#include "core/utils/type.h"

namespace allpix {
    /**
     * @ingroup Exceptions
     * @brief Receive of a message that was not expected
     *
     * Raised if a module receives a message again while its \ref SingleBindDelegate "bound variable" is
     * already pointing to the earlier received message.
     */
    // TODO [doc] Should be renamed to UnexpectedMessageError
    class UnexpectedMessageException : public RuntimeError {
    public:
        /**
         * @brief Constructs an error with a received message
         * @param module Receiving module
         * @param message Type of the received message
         */
        UnexpectedMessageException(const std::string& module, const std::type_info& message) {
            // FIXME: add detector and input output instance here
            error_message_ = "Unexpected message ";
            error_message_ += allpix::demangle(message.name());
            error_message_ += " received by module " + module + " (only a single one expected per event)";
        }
    };

    /**
     * @ingroup Exceptions
     * @brief Message does not contain an \ref allpix::Object
     */
    class MessageWithoutObjectException : public RuntimeError {
    public:
        /**
         * @brief Constructs an error for a message without an object
         * @param message Type of the received message
         */
        explicit MessageWithoutObjectException(const std::type_info& message) {
            error_message_ = "Message ";
            error_message_ += allpix::demangle(message.name());
            error_message_ += " does not contain a valid object";
        }
    };

    /**
     * @ingroup Exceptions
     * @brief Trying to fetch a message that wasn't delivered
     *
     * Raised if a module tries to fetch a message that it didn't receive
     */
    class MessageNotFoundException : public RuntimeError {
    public:
        explicit MessageNotFoundException(const std::string& module, const std::type_info& message) {
            error_message_ = "Module " + module + " did not receive message ";
            error_message_ += allpix::demangle(message.name());
        }
    };
} // namespace allpix

#endif /* ALLPIX_MESSENGER_EXCEPTIONS_H */
