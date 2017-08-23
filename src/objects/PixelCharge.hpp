/**
 * @file
 * @brief Definition of object with set of particles at pixel
 * @copyright MIT License
 */

#ifndef ALLPIX_PIXEL_CHARGE_H
#define ALLPIX_PIXEL_CHARGE_H

#include <Math/DisplacementVector2D.h>

#include <TRefArray.h>

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
         * @brief ROOT class definition
         */
        ClassDef(PixelCharge, 2);
        /**
         * @brief Default constructor for ROOT I/O
         */
        PixelCharge() = default;

    private:
        Pixel pixel_;
        unsigned int charge_{};

        TRefArray propagated_charges_;
    };

    /**
     * @brief Typedef for message carrying pixel charges
     */
    using PixelChargeMessage = Message<PixelCharge>;
} // namespace allpix

#endif
