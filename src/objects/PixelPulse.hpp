/**
 * @file
 * @brief Definition of pixel pulse object
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_PIXEL_PULSE_H
#define ALLPIX_PIXEL_PULSE_H

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
    class PixelPulse : public Object {
    public:
        /**
         * @brief Construct a pixel pulse
         * @param pixel Object holding the information of the pixel
         * @param time_resolution Time per pulse bin, i.e. resolution
         * @param time_total Total integration time of the pulse
         */
        PixelPulse(Pixel pixel, double time_resolution, double time_total);

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
        ClassDef(PixelPulse, 1);
        /**
         * @brief Default constructor for ROOT I/O
         */
        PixelPulse() = default;

    private:
        Pixel pixel_;
        double resolution_;
        std::vector<double> pulse_;

        TRefArray deposited_charges_;
    };

    /**
     * @brief Typedef for message carrying deposits
     */
    using PixelPulseMessage = Message<PixelPulse>;
} // namespace allpix

#endif
