/**
 * @file
 * @brief Definition of pulse object
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_PULSE_H
#define ALLPIX_PULSE_H

#include <Math/DisplacementVector2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <TObject.h>

namespace allpix {
    /**
     * @ingroup Objects
     * @brief Pulse holding induced charges as a function of time
     * @warning This object is special and is not meant to be written directly to a tree (not inheriting from \ref Object)
     */
    class Pulse {
    public:
        /**
         * @brief Construct a new pulse
         */
        Pulse(double integration_time, double time_bin);

        /**
         * @brief adding induced charge to the pulse
         * @param charge induced charge
         * @param time   time when it has been induced
         */
        void addCharge(double charge, double time);

        /**
         * @brief Function to retrieve the integral (net) charge from the full pulse
         * @return Integrated charge
         */
        unsigned int getCharge();

        /**
         * @brief ROOT class definition
         */
        Pulse() = default;
        /**
         * @brief Default constructor for ROOT I/O
         */
        ClassDef(Pulse, 1);

    private:
        std::vector<double> pulse_;
        double time_, bin_;
    };

} // namespace allpix

#endif
