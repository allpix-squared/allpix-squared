/**
 * AllPix messenger exception classes
 *
 * @author Simon Spannagel <simon.spannagel@cern.ch>
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MESSENGER_EXCEPTIONS_H
#define ALLPIX_MESSENGER_EXCEPTIONS_H

#include <string>

#include "core/utils/exceptions.h"
#include "core/utils/type.h"

namespace allpix {
    /*
     * Receive of a message that a module did not expect (only one message)
     */
    class UnexpectedMessageException : public RuntimeError {
    public:
        UnexpectedMessageException(const std::string& module, const std::type_info& message) {
            // FIXME: add detectory and input output instance here
            error_message_ = "Unexpected message ";
            error_message_ += allpix::demangle(message.name());
            error_message_ += " received by module " + module + " (only a single one expected per event)";
        }
    };
} // namespace allpix

#endif /* ALLPIX_MESSENGER_EXCEPTIONS_H */
