/**
 * @file
 * @brief Collection of all object exceptions
 *
 * @copyright MIT License
 */

#ifndef ALLPIX_OBJECT_EXCEPTIONS_H
#define ALLPIX_OBJECT_EXCEPTIONS_H

#include <string>

#include "core/utils/exceptions.h"
#include "core/utils/type.h"

namespace allpix {
    /**
     * @ingroup Exceptions
     * @brief Indicates an object that does not contain a reference fetched
     */
    class MissingReferenceException : public RuntimeError {
    public:
        /**
         * @brief Constructs an error for a object with missing reference
         * @param source Type of the object from which the reference was requested
         * @param reference Type of the non-existing reference
         */
        explicit MissingReferenceException(const std::type_info& source, const std::type_info& reference) {
            error_message_ = "Object ";
            error_message_ += allpix::demangle(source.name());
            error_message_ += " is missing reference to ";
            error_message_ += allpix::demangle(reference.name());
        }
    };
} // namespace allpix

#endif /* ALLPIX_OBJECT_EXCEPTIONS_H */
