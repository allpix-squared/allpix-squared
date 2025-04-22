/**
 * @file
 * @brief Definition of pixel object
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef CORRYVRECKAN_PIXEL_H
#define CORRYVRECKAN_PIXEL_H 1

#include "Object.hpp"

namespace corryvreckan {
    /**
     * @ingroup Objects
     * @brief Pixel hit object
     */
    class Pixel : public Object {

    public:
        // Constructors and destructors
        /**
         * @brief Required default constructor
         */
        Pixel() = default;

        /**
         * @brief Static member function to obtain base class for storage on the clipboard.
         * This method is used to store objects from derived classes under the typeid of their base classes
         *
         * @warning This function should not be implemented for derived object classes
         *
         * @return Class type of the base object
         */
        static std::type_index getBaseType() { return typeid(Pixel); }

        /**
         * @brief Class constructor
         * @param detectorID detectorID
         * @param col Pixel column
         * @param row Pixel row
         * @param timestamp Pixel timestamp in nanoseconds
         * @param raw Charge-equivalent pixel raw value. If not available set to 1.
         * @param charge Pixel charge in electrons. If not available, set to raw for correct charge-weighted clustering.
         *
         * `column` and `row` correspond to the position of a hit. `raw` is a generic charge equivalent pixel value which can
         * be `ToT`, `ADC`, etc., depending on the detector. `charge` is the total charge of the integrated signal of a hit
         * and the `timestamp` is pixel timestamp. Not all of these values are available for all detector types. If a
         * detector doesn't provide a pixel `timestamp`, for instance, it should simply be set to 0. As cluster (spatial as
         * well as 4D) are charge-weighting the cluster position the `charge` should be set to `raw` if no other information
         * is available. If `raw` is not available either, it should be set to 1.
         */
        Pixel(std::string detectorID, int col, int row, int raw, double charge, double timestamp)
            : Object(std::move(detectorID), timestamp), m_column(col), m_row(row), m_raw(raw), m_charge(charge) {}

        // Methods to get member variables:
        /**
         * @brief Get pixel row
         * @return Pixel row
         */
        int row() const { return m_row; }
        /**
         * @brief Get pixel column
         * @return Pixel column
         */
        int column() const { return m_column; }
        /**
         * @brief Get pixel coordinates
         * @return Pixel coordinates (column, row)
         */
        std::pair<int, int> coordinates() const { return std::make_pair(m_column, m_row); }

        /**
         * @brief Get pixel raw value (charge equivalent depending on detector, e.g. ToT, ADC, ...)
         * @return Pixel raw value
         *
         * raw is a generic charge equivalent pixel value which can be ToT, ADC, ..., depending on the detector
         * if isBinary==true, the value will always be 1 and shouldn't be used for anything
         */
        int raw() const { return m_raw; }
        /**
         * @brief Get pixel charge in electrons
         * @return Pixel charge
         */
        double charge() const { return m_charge; }

        // Methods to set member variables:
        /**
         * @brief Set pixel raw value (charge equivalent depending on detector, e.g. ToT, ADC, ...)
         */
        void setRaw(int raw) { m_raw = raw; }
        /**
         * @brief Set pixel charge in electrons
         */
        void setCharge(double charge) { m_charge = charge; }

        /**
         * @brief Print an ASCII representation of Pixel to the given stream
         * @param out Stream to print to
         */
        void print(std::ostream& out) const override;

        /**
         * @brief ROOT class definition
         */
        ClassDefOverride(Pixel, 7); // NOLINT

    private:
        // Member variables
        int m_column;
        int m_row;
        int m_raw;
        double m_charge;
    };

    // Vector type declaration
    using PixelVector = std::vector<Pixel*>;
} // namespace corryvreckan

#endif // CORRYVRECKAN_PIXEL_H
