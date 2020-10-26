/**
 * @file
 * @brief Definition of object with set of particles at pixel
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_PIXEL_CHARGE_H
#define ALLPIX_PIXEL_CHARGE_H

#include <Math/DisplacementVector2D.h>
#include <TRef.h>
#include <algorithm>

#include "MCParticle.hpp"
#include "Object.hpp"
#include "Pixel.hpp"
#include "PropagatedCharge.hpp"
#include "Pulse.hpp"

namespace allpix {
    /**
     * @ingroup Objects
     * @brief Set of charges at a pixel
     */
    class PixelCharge : public Object {
        friend class PixelHit;

    public:
        /**
         * @brief Construct a set of charges at a pixel
         * @param pixel Object holding the information of the pixel
         * @param charge Amount of charge stored at this pixel
         * @param propagated_charges Optional pointer to the related propagated charges
         */
        PixelCharge(Pixel pixel,
                    long charge,
                    const std::vector<const PropagatedCharge*>& propagated_charges = std::vector<const PropagatedCharge*>());

        /**
         * @brief Construct a set of charges at a pixel
         * @param pixel Object holding the information of the pixel
         * @param pulse Pulse of induced or collected charges
         * @param propagated_charges Optional pointer to the related propagated charges
         */
        PixelCharge(Pixel pixel,
                    Pulse pulse,
                    const std::vector<const PropagatedCharge*>& propagated_charges = std::vector<const PropagatedCharge*>());

        /**
         * @brief Get the pixel containing the charges
         * @return Pixel indices in the grid
         */
        const Pixel& getPixel() const;

        /**
         * @brief Shortcut to retrieve the pixel indices
         * @return Index of the pixel
         */
        Pixel::Index getIndex() const;

        /**
         * @brief Get the charge at the pixel
         * @return Total charge stored
         */
        long getCharge() const;

        /**
         * @brief Get the absolute charge value at the pixel
         * @return Absolute total charge stored
         */
        unsigned long getAbsoluteCharge() const;

        /**
         * @brief Get related propagated charges
         * @return Possible set of pointers to propagated charges
         */
        std::vector<const PropagatedCharge*> getPropagatedCharges() const;

        /**
         * @brief Get the Monte-Carlo particles resulting in this pixel hit
         * @return List of all related Monte-Carlo particles
         */
        std::vector<const MCParticle*> getMCParticles() const;

        /**
         *  @brief Get recoded charge pulse
         *  @return Constatnt reference to the full charge pulse
         */
        const Pulse& getPulse() const;

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
         * @brief Print an ASCII representation of PixelCharge to the given stream
         * @param out Stream to print to
         */
        void print(std::ostream& out) const override;

        /**
         * @brief ROOT class definition
         */
        ClassDefOverride(PixelCharge, 8);
        /**
         * @brief Default constructor for ROOT I/O
         */
        PixelCharge() = default;

        void petrifyHistory() override;

    private:
        Pixel pixel_;
        long charge_{};
        Pulse pulse_{};

        double local_time_{};
        double global_time_{};

        std::vector<PointerWrapper<PropagatedCharge>> propagated_charges_;
        std::vector<PointerWrapper<MCParticle>> mc_particles_;
    };

    /**
     * @brief Typedef for message carrying pixel charges
     */
    using PixelChargeMessage = Message<PixelCharge>;
} // namespace allpix

#endif /* ALLPIX_PIXEL_CHARGE_H */
