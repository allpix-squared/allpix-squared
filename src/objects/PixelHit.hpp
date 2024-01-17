/**
 * @file
 * @brief Definition of object with digitized pixel hit
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_PIXEL_HIT_H
#define ALLPIX_PIXEL_HIT_H

#include <Math/DisplacementVector2D.h>

#include <TRef.h>

#include "MCParticle.hpp"
#include "Object.hpp"
#include "PixelCharge.hpp"
#include "PixelPulse.hpp"

#include "Pixel.hpp"

namespace allpix {
    /**
     * @ingroup Objects
     * @brief Pixel triggered in an event after digitization
     */
    class PixelHit : public Object {
    public:
        /**
         * @brief Construct a digitized pixel hit
         * @param pixel Object holding the information of the pixel
         * @param local_time Timing of the occurrence of the hit in local reference frame
         * @param global_time Timing of the occurrence of the hit in global reference frame
         * @param signal Signal data produced by the digitizer
         * @param pixel_charge Optional pointer to the related pixel charge
         * @param pixel_pulse Optional pointer to the related pixel pulse this his might have been derived from
         */
        PixelHit(Pixel pixel,
                 double local_time,
                 double global_time,
                 double signal,
                 const PixelCharge* pixel_charge = nullptr,
                 const PixelPulse* pixel_pulse = nullptr);

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
         * @brief Get the timing data of the hit in the global reference frame
         * @return Timestamp from event start
         */
        double getGlobalTime() const { return global_time_; }

        /**
         * @brief Get the timing data of the hit in the local reference frame
         * @return Timestamp from primary particle arrival
         */
        double getLocalTime() const { return local_time_; }

        /**
         * @brief Get the signal data for the hit
         * @return Digitized signal
         */
        double getSignal() const { return signal_; }

        /**
         * @brief Get related pixel charge
         * @return Possible related pixel charge
         */
        const PixelCharge* getPixelCharge() const;

        /**
         * @brief Get related pixel pulse
         * @return Possible related pixel pulse
         */
        const PixelPulse* getPixelPulse() const;

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
         * @brief Print an ASCII representation of PixelHit to the given stream
         * @param out Stream to print to
         */
        void print(std::ostream& out) const override;

        /**
         * @brief ROOT class definition
         */
        ClassDefOverride(PixelHit, 7); // NOLINT
        /**
         * @brief Default constructor for ROOT I/O
         */
        PixelHit() = default;

        void loadHistory() override;
        void petrifyHistory() override;

    private:
        Pixel pixel_;
        double local_time_{};
        double global_time_{};
        double signal_{};

        PointerWrapper<PixelCharge> pixel_charge_;
        PointerWrapper<PixelPulse> pixel_pulse_;
        std::vector<PointerWrapper<MCParticle>> mc_particles_;
    };

    /**
     * @brief Typedef for message carrying pixel hits
     */
    using PixelHitMessage = Message<PixelHit>;
} // namespace allpix

#endif /* ALLPIX_PIXEL_HIT_H */
