/**
 * @file
 * @brief Definition of pixel object
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_PIXEL_H
#define ALLPIX_PIXEL_H

#include <Math/DisplacementVector2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <TObject.h>

namespace ROOT::Math { // NOLINT
    bool operator<(const ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>>& lhs,
                   const ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>>& rhs);
} // namespace ROOT::Math

namespace allpix {
    /**
     * @ingroup Objects
     * @brief Pixel in the model with indices, location and size
     * @warning This object is special and is not meant to be written directly to a tree (not inheriting from \ref Object)
     */
    class Pixel {
    public:
        using Index = ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>>;

        /**
         * @brief Type of pixels
         */
        enum class Type {
            RECTANGLE = 0,  ///< Rectangular pixel shape
            HEXAGON_FLAT,   ///< Hexagonal pixel shape, flat side up
            HEXAGON_POINTY, ///< Hexagonal pixel shape, corner up
        };

        /**
         * @brief Construct a new pixel
         */
        Pixel(Pixel::Index index,
              Pixel::Type type,
              ROOT::Math::XYZPoint local_center,
              ROOT::Math::XYZPoint global_center,
              ROOT::Math::XYVector size);

        /**
         * @brief Return index pair of pixel
         * @return Index in x,y-plane
         */
        Pixel::Index getIndex() const;

        /**
         * @brief Return type of pixel
         * @return Type of the pixel describing its shape and orientation
         */
        Pixel::Type getType() const;

        /**
         * @brief Get center position in local coordinates
         * @return Local center position
         */
        ROOT::Math::XYZPoint getLocalCenter() const;
        /**
         * @brief Get center position in global coordinates
         * @return Global center position
         */
        ROOT::Math::XYZPoint getGlobalCenter() const;
        /**
         * @brief Get size of pixel
         * @return Pixel size
         */
        ROOT::Math::XYVector getSize() const;

        /**
         * @brief ROOT class definition
         */
        Pixel() = default;
        /**
         * @brief Default constructor for ROOT I/O
         */
        ClassDef(Pixel, 2); // NOLINT

    private:
        Pixel::Index index_;
        Pixel::Type type_{};

        ROOT::Math::XYZPoint local_center_;
        ROOT::Math::XYZPoint global_center_;
        ROOT::Math::XYVector size_;
    };

} // namespace allpix

#endif /* ALLPIX_PIXEL_H */
