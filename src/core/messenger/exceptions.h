/**
 * @file
 * @brief Collection of all message exceptions
 *
 * @copyright MIT License
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
     * @brief Message does not contain an \ref Object
     */
    class MessageWithoutObjectException : public RuntimeError {
    public:
        /**
         * @brief Constructs an error for a message without an object
         * @param message Type of the received message
         */
        MessageWithoutObjectException(const std::type_info& message) {
            error_message_ = "Message ";
            error_message_ += allpix::demangle(message.name());
            error_message_ += " does not contain an AllPix object";
        }
    };
} // namespace allpix

#endif /* ALLPIX_MESSENGER_EXCEPTIONS_H */
