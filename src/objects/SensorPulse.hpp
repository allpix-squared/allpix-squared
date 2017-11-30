/**
 * @file
 * @brief Definition of sensor pulse object
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_SENSOR_PULSE_H
#define ALLPIX_SENSOR_PULSE_H

#include <TRefArray.h>

#include "DepositedCharge.hpp"
#include "MCParticle.hpp"
#include "Object.hpp"
#include "Pixel.hpp"

namespace allpix {
    /**
     * @ingroup Objects
     * @brief Current pulse at sensor implant of the detector
     */
    class SensorPulse : public Object {
    public:
        /**
         * @brief Construct a sensor pulse
         * @param pixel Object holding the information of the pixel
         * @param time_resolution Time per pulse bin, i.e. resolution
         * @param time_total Total integration time of the pulse
         */
        SensorPulse(Pixel pixel, double time_resolution, double time_total);

        /**
         * @brief Add new current contribution
         * @param type Type of the contributing charge carrier
         * @param time Time of the induced current
         * @param current Induced current
         * @param deposited_charge Deposited charge creating the induced current
         */
        void addCurrent(CarrierType type, double time, double current, const DepositedCharge* deposited_charge = nullptr);

        /**
         * @brief Get related deposited charges
         * @return Possible set of pointers to deposited charges
         */
        std::vector<const DepositedCharge*> getDepositedCharges() const;

        /**
         * @brief ROOT class definition
         */
        ClassDef(SensorPulse, 1);
        /**
         * @brief Default constructor for ROOT I/O
         */
        SensorPulse() = default;

    private:
        Pixel pixel_;
        double resolution_;
        std::vector<double> pulse_;

        TRefArray deposited_charges_;
    };

    /**
     * @brief Typedef for message carrying deposits
     */
    using SensorPulseMessage = Message<SensorPulse>;
} // namespace allpix

#endif
