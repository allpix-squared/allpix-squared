/**
 * @file
 * @brief Definition of Object base class
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

/**
 * @defgroup Objects Objects
 * @brief Collection of objects passed around between modules
 */

#ifndef CORRYVRECKAN_OBJECT_H
#define CORRYVRECKAN_OBJECT_H

#include <TTree.h>
#include <iostream>
#include <string>
#include <typeindex>
#include <vector>
#include "TObject.h"

namespace corryvreckan {

    /**
     * @ingroup Objects
     * @brief Base class for internal objects
     *
     * Generic base class. Every class which inherits from Object can be placed on the clipboard and written out to file.
     */
    class Object : public TObject {

    public:
        friend std::ostream& operator<<(std::ostream& out, const corryvreckan::Object& obj);

        /**
         * @brief Required default constructor
         */
        Object();
        explicit Object(std::string detectorID);
        explicit Object(double timestamp);
        Object(std::string detectorID, double timestamp);
        Object(const Object&);

        /**
         * @brief Required virtual destructor
         */
        ~Object() override;

        // Methods to get member variables
        const std::string& getDetectorID() const { return m_detectorID; }
        const std::string& detectorID() const { return getDetectorID(); }

        double timestamp() const { return m_timestamp; }
        void timestamp(double time) { m_timestamp = time; }
        void setTimestamp(double time) { timestamp(time); }

        // Methods to set member variables
        void setDetectorID(std::string detectorID) { m_detectorID = std::move(detectorID); }

        /**
         * @brief ROOT class definition
         */
        ClassDefOverride(Object, 5); // NOLINT

    protected:
        // Member variables
        std::string m_detectorID;
        double m_timestamp{0};

        /**
         * @brief Print an ASCII representation of this Object to the given stream
         * @param out Stream to print to
         */
        virtual void print(std::ostream& out) const { out << "<unknown object>"; };

        /**
         * @brief Override function to implement ROOT Print()
         * @warning Should not be used inside the framework but might assist in inspecting ROOT files with these objects.
         */
        void Print(Option_t*) const override {
            print(std::cout);
            std::cout << '\n';
        }
    };

    /**
     * @brief Overloaded ostream operator for printing of object data
     * @param out Stream to write output to
     * @param obj Object to print to stream
     * @return Stream where output was written to
     */
    std::ostream& operator<<(std::ostream& out, const corryvreckan::Object& obj);

    // Vector type declaration
    using ObjectVector = std::vector<Object*>;
} // namespace corryvreckan

#endif // CORRYVRECKAN_OBJECT_H
