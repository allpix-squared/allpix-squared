/**
 * AllPix geometry exception classes
 *
 * @author Simon Spannagel <simon.spannagel@cern.ch>
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_GEOMETRY_EXCEPTIONS_H
#define ALLPIX_GEOMETRY_EXCEPTIONS_H

#include <string>

#include "core/utils/exceptions.h"
#include "core/utils/type.h"

namespace allpix {
    /*
     * Errors related to detectors that do not exist
     *
     * FIXME: specialize and generalize these errors?
     */
    class InvalidDetectorError : public RuntimeError {
    public:
        InvalidDetectorError(const std::string& category, const std::string& detector) {
            error_message_ = "Could not find a detector with " + category + " '" + detector + "'";
        }
    };
    class DetectorNameExistsError : public RuntimeError {
    public:
        explicit DetectorNameExistsError(const std::string& name) {
            error_message_ = "Detector with name " + name + " is already registered, detector names should be unique";
        }
    };
}

#endif /* ALLPIX_GEOMETRY_EXCEPTIONS_H */
