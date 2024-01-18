/**
 * @file
 * @brief Definition of object with pulse processed by pixel front-end
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_PIXEL_PULSE_H
#define ALLPIX_PIXEL_PULSE_H

#include <Math/DisplacementVector2D.h>

#include <TRef.h>

#include "MCParticle.hpp"
#include "Object.hpp"
#include "PixelCharge.hpp"
#include "Pulse.hpp"

#include "Pixel.hpp"

namespace allpix {
    /**
     * @ingroup Objects
     * @brief Pixel triggered in an event after digitization
     */
    class PixelPulse : public Object, public Pulse {
    public:
        /**
         * @brief Construct a digitized pixel front-end pulse
         * @param pixel Object holding the information of the pixel
         * @param pulse Output pulse produced by the digitizer
         * @param pixel_charge Optional pointer to the related pixel charge
         */
        PixelPulse(Pixel pixel, const Pulse& pulse, const PixelCharge* pixel_charge = nullptr);

        /**
         * @brief Get the pixel hit
         * @return Pixel indices in the grid
         */
        const Pixel& getPixel() const;

        /**
         * @brief Shortcut to retrieve the pixel indices
         * @return Index of the pixel
         */
        Pixel::Index getIndex() const;

        /**
         * @brief Get related pixel charge
         * @return Possible related pixel charge
         */
        const PixelCharge* getPixelCharge() const;

        /**
         * @brief Get the Monte-Carlo particles resulting in this pixel hit
         * @return List of all related Monte-Carlo particles
         */
        std::vector<const MCParticle*> getMCParticles() const;

        /**
         * @brief Get all primary Monte-Carlo particles resulting in this pixel hit. A particle is considered primary if it
         * has no parent particle set.
         * @return List of all related primary Monte-Carlo particles
         */
        std::vector<const MCParticle*> getPrimaryMCParticles() const;

        /**
         * @brief Get time after start of event in global reference frame
         * @return Time from start event
         */
        double getGlobalTime() const;

        /**
         * @brief Get local time in the sensor
         * @return Time with respect to local sensor
         */
        double getLocalTime() const;

        /**
         * @brief Print an ASCII representation of PixelHit to the given stream
         * @param out Stream to print to
         */
        void print(std::ostream& out) const override;

        /**
         * @brief ROOT class definition
         */
        ClassDefOverride(PixelPulse, 1); // NOLINT
        /**
         * @brief Default constructor for ROOT I/O
         */
        PixelPulse() = default;

        void loadHistory() override;
        void petrifyHistory() override;

    private:
        Pixel pixel_;

        double local_time_{};
        double global_time_{};

        PointerWrapper<PixelCharge> pixel_charge_;
        std::vector<PointerWrapper<MCParticle>> mc_particles_;
    };

    /**
     * @brief Typedef for message carrying pixel pulses
     */
    using PixelPulseMessage = Message<PixelPulse>;
} // namespace allpix

#endif /* ALLPIX_PIXEL_PULSE_H */
