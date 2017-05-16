/**
 * @file
 * @brief Collection of all geometry exceptions
 *
 * @copyright MIT License
 */

#ifndef ALLPIX_GEOMETRY_EXCEPTIONS_H
#define ALLPIX_GEOMETRY_EXCEPTIONS_H

#include <string>

#include "core/utils/exceptions.h"
#include "core/utils/type.h"

namespace allpix {
    /**
     * @ingroup Exceptions
     * @brief Indicates an error with finding a detector by name or by type
     */
    // TODO [doc] Split this up in a detector and invalid model and rename to DetectorNotFoundError?
    class InvalidDetectorError : public RuntimeError {
    public:
        /**
         * @brief Constructs an error with a detector that is not found
         * @param category Either 'type' for a model or 'name' for detector
         * @param detector Identifier for the detector that is not found
         */
        InvalidDetectorError(const std::string& category, const std::string& detector) {
            error_message_ = "Could not find a detector with " + category + " '" + detector + "'";
        }
    };

    /**
     * @ingroup Exceptions
     * @brief Indicates an attempt to add a detector that is already registered before
     */
    class DetectorNameExistsError : public RuntimeError {
    public:
        /**
         * @brief Constructs an error for a non unique detector
         * @param name Name of the detector that is added earlier
         */
        explicit DetectorNameExistsError(const std::string& name) {
            error_message_ = "Detector with name " + name + " is already registered, detector names should be unique";
        }
    };
} // namespace allpix

#endif /* ALLPIX_GEOMETRY_EXCEPTIONS_H */
