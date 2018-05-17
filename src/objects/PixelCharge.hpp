/**
 * @file
 * @brief Definition of object with set of particles at pixel
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
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

namespace allpix {
    /**
     * @ingroup Objects
     * @brief Set of charges at a pixel
     */
    class PixelCharge : public Object {
    public:
        /**
         * @brief Construct a set of charges at a pixel
         * @param pixel Object holding the information of the pixel
         * @param charge Amount of charge stored at this pixel
         * @param propagated_charges Optional pointer to the related propagated charges
         */
        PixelCharge(Pixel pixel,
                    unsigned int charge,
                    std::vector<const PropagatedCharge*> propagated_charges = std::vector<const PropagatedCharge*>());

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
        unsigned int getCharge() const;

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
         * @brief ROOT class definition
         */
        ClassDefOverride(PixelCharge, 4);
        /**
         * @brief Default constructor for ROOT I/O
         */
        PixelCharge() = default;

    private:
        Pixel pixel_;
        unsigned int charge_{};

        std::vector<TRef> propagated_charges_;
        std::vector<TRef> mc_particles_;
    };

    /**
     * @brief Typedef for message carrying pixel charges
     */
    using PixelChargeMessage = Message<PixelCharge>;
} // namespace allpix

#endif
