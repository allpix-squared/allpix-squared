/**
 * @file
 * @brief Defines 2D Geant4 parameterization grid of elements
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_PARAMETERIZATION_2D_HH_
#define ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_PARAMETERIZATION_2D_HH_

#include <memory>

#include <G4PVParameterised.hh>
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

        /**
         * @brief Place the physical volume at the correct place with the copy number
         * @param copy_id Id of the volume on the grid
         * @param phys_volume Physical volume to place on the grid
         */
        void ComputeTransformation(const G4int, G4VPhysicalVolume*) const override;

    private:
        int div_x_;
        double size_x_;
        double size_y_;
        double offset_x_;
        double offset_y_;
        double pos_z_;
    };

    /**
     * @brief Class to construct parameterized physical volumes allowing to switch off overlap checking
     * @warning This overload is needed to allow to disable overlap checking, because it can hang the deposition
     */
    class ParameterisedG4 : public G4PVParameterised {
    public:
        ParameterisedG4(const G4String& name,
                        G4LogicalVolume* logical,
                        G4LogicalVolume* mother,
                        const EAxis axis,
                        const int n_replicas,
                        G4VPVParameterisation* param,
                        bool check_overlaps);

        /**
         * @warning This method is overwritten to allow to disable it, because the overlap checking can hang the deposition
         */
        bool CheckOverlaps(int res, double tol, bool verbose, int max_err) override;

    private:
        bool check_overlaps_;
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_PARAMETERIZATION_2D_HH_ */
