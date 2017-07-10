/**
 * @file
 * @brief Definition of pixel object
 * @copyright MIT License
 */

#ifndef ALLPIX_PIXEL_H
#define ALLPIX_PIXEL_H

#include <Math/DisplacementVector2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <TObject.h>

namespace allpix {
    /**
     * @ingroup Objects
     * @brief Pixel in the model with indices, location and size
     * @warning This object is special and is not meant to be written directly to a tree (not inheriting from \ref Object)
     */
    class Pixel {
    public:
        using Index = ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<unsigned int>>;

        /**
         * @brief Construct a new pixel
         */
        Pixel(Pixel::Index index,
              ROOT::Math::XYZPoint local_center,
              ROOT::Math::XYZPoint global_center,
              ROOT::Math::XYVector size);

        /**
         * @brief Return index pair of pixel
         * @return Index in x,y-plane
         */
        Pixel::Index getIndex() const;

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
        ClassDef(Pixel, 1);

    private:
        Pixel::Index index_;

        ROOT::Math::XYZPoint local_center_;
        ROOT::Math::XYZPoint global_center_;
        ROOT::Math::XYVector size_;
    };
} // namespace allpix

#endif
