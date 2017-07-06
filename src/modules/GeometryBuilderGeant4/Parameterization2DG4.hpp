/**
 * @file
 * @brief Defines 2D Geant4 parameterization grid of elements
 * @copyright MIT License
 */

#ifndef ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_PARAMETERIZATION_2D_HH_
#define ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_PARAMETERIZATION_2D_HH_

#include <memory>

#include <G4ThreeVector.hh>
#include <G4VPVParameterisation.hh>
#include <G4VPhysicalVolume.hh>

namespace allpix {
    /**
     * @brief Represents a 2D Geant4 parameterization in the X,Y plane
     *
     * Used to construct the pixel grid and the array of bump bonds.
     */
    class Parameterization2DG4 : public G4VPVParameterisation {
    public:
        /**
         * @brief Construct the parameterization
         * @param div_x Number of divisions in the x-direction (y is automatically inferred)
         * @param size_x Size of single element in x-direction
         * @param size_y Size of single element in y-direction
         * @param offset_x Offset of grid in the x-direction
         * @param offset_y Offset of grid in the y-direction
         * @param pos_z Position of the 2D parameterization in the z-axis
         */
        Parameterization2DG4(int div_x, double size_x, double size_y, double offset_x, double offset_y, double pos_z);

        void ComputeTransformation(int, G4VPhysicalVolume*) const override;

    private:
        int div_x_;
        double size_x_;
        double size_y_;
        double offset_x_;
        double offset_y_;
        double pos_z_;
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_PARAMETERIZATION_2D_HH_ */
