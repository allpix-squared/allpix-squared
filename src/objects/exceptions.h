/**
 * @file
 * @brief Collection of all object exceptions
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
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

    /**
     * @ingroup Exceptions
     * @brief Indicates that two objects are of incompatible data types and cannot be combined
     */
    class IncompatibleDatatypesException : public RuntimeError {
    public:
        /**
         * @brief Constructs an error for two objects with incompatible data types
         * @param source1 Type of the first object
         * @param source2 Type of the second object
         * @param reason Reason why the types are incompatible
         */
        explicit IncompatibleDatatypesException(const std::type_info& source1,
                                                const std::type_info& source2,
                                                const std::string& reason) {
            error_message_ = "Objects ";
            error_message_ += allpix::demangle(source1.name());
            error_message_ += " and ";
            error_message_ += allpix::demangle(source2.name());
            error_message_ += " have incompatible types";

            if(!reason.empty()) {
                error_message_ += ": " + reason;
            }
        }
    };

    /**
     * @ingroup Exceptions
     * @brief Indicates that a pulse object could not be allocated
     */
    class PulseBadAllocException : public RuntimeError {
    public:
        /**
         * @brief Constructs an error for a pulse which could not be extended to the requested size
         * @param bins Number of bins attempted to allocate
         * @param time Total integration time of the pulse
         * @param reason Reason why the allocation failed
         */
        explicit PulseBadAllocException(const size_t bins, const double time, const std::string& reason) {
            error_message_ = "Unable to allocate memory for pulse with ";
            error_message_ += std::to_string(bins);
            error_message_ += " bins and total duration of ";
            error_message_ += std::to_string(time);
            error_message_ += "ns";

            if(!reason.empty()) {
                error_message_ += ": " + reason;
            }
        }
    };
} // namespace allpix

#endif /* ALLPIX_OBJECT_EXCEPTIONS_H */
