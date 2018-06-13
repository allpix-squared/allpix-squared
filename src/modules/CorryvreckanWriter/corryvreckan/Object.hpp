/**
 * @file
 * @brief Definition of Object base class
 * @copyright Copyright (c) 2017 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

/**
 * @defgroup Objects Objects
 * @brief Collection of objects passed around between modules
 */

#ifndef CORRYVRECKAN_OBJECT_H
#define CORRYVRECKAN_OBJECT_H

#include <string>
#include <vector>
#include "TObject.h"
#include "TTree.h"

namespace corryvreckan {

    /**
     * @ingroup Objects
     * @brief Base class for internal objects
     *
     * Generic base class. Every class which inherits from Object can be placed on the clipboard and written out to file.
     */
    class Object : public TObject {

    public:
        // Constructors and destructors
        Object();
        explicit Object(std::string detectorID);
        explicit Object(double timestamp);
        Object(std::string detectorID, double timestamp);
        ~Object() override;

        // Methods to get member variables
        std::string getDetectorID() { return m_detectorID; }
        std::string detectorID() { return getDetectorID(); }

        double timestamp() { return m_timestamp; }
        void timestamp(double time) { m_timestamp = time; }
        void setTimestamp(double time) { timestamp(time); }

        // Methods to set member variables
        void setDetectorID(std::string detectorID) { m_detectorID = std::move(detectorID); }

    protected:
        // Member variables
        std::string m_detectorID;
        double m_timestamp{0};

        // ROOT I/O class definition - update version number when you change this class!
        ClassDefOverride(Object, 2)
    };

    // Vector type declaration
    using Objects = std::vector<Object*>;
} // namespace corryvreckan

#endif // CORRYVRECKAN_OBJECT_H
