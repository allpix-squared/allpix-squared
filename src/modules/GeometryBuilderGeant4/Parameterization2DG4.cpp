/**
 * @file
 * @brief Implements 2D Geant4 parameterization grid of elements
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "Parameterization2DG4.hpp"

#include <memory>

using namespace allpix;

Parameterization2DG4::Parameterization2DG4(
    int div_x, double size_x, double size_y, double offset_x, double offset_y, double pos_z)
    : div_x_(div_x), size_x_(size_x), size_y_(size_y), offset_x_(offset_x), offset_y_(offset_y), pos_z_(pos_z) {}

void Parameterization2DG4::ComputeTransformation(const G4int copy_id, G4VPhysicalVolume* phys_volume) const {
    auto idx_x = copy_id % div_x_;
    auto idx_y = copy_id / div_x_;

    auto pos_x = (idx_x + 0.5) * size_x_ + offset_x_;
    auto pos_y = (idx_y + 0.5) * size_y_ + offset_y_;

    phys_volume->SetTranslation(G4ThreeVector(pos_x, pos_y, pos_z_));
    phys_volume->SetRotation(nullptr);
}

ParameterisedG4::ParameterisedG4(const G4String& name,
                                 G4LogicalVolume* logical,
                                 G4LogicalVolume* mother,
                                 const EAxis axis,
                                 const int n_replicas,
                                 G4VPVParameterisation* param,
                                 bool check_overlaps)
    : G4PVParameterised(name, logical, mother, axis, n_replicas, param, check_overlaps), check_overlaps_(check_overlaps) {}

/**
 * @warning This method is overwritten to allow to disable it, because the overlap checking can hang the deposition
 */
bool ParameterisedG4::CheckOverlaps(int res, double tol, bool verbose, int max_err) {
    if(check_overlaps_) {
        G4PVParameterised::CheckOverlaps(res, tol, verbose, max_err);
    }
    return false;
}
