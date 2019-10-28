/**
 * @file
 * @brief Implementation of detector model
 *
 * @copyright Copyright (c) 2017-2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "PassiveMaterialModel.hpp"
#include "core/config/Configuration.hpp"
#include "core/module/exceptions.h"

#include <G4Box.hh>
#include "G4LogicalVolume.hh"
#include "G4VSolid.hh"

using namespace allpix;

PassiveMaterialModel::PassiveMaterialModel(Configuration&) {
    auto size = ROOT::Math::XYZVector();
    solid_ = new G4Box("name", size.x(), size.y(), size.z());
    filling_solid_ = new G4Box("filling_name", size.x(), size.y(), size.z());
    max_size_ = std::max(size.x(), std::max(size.y(), size.z()));
}

G4VSolid* PassiveMaterialModel::GetSolid() {
    return solid_;
}

G4VSolid* PassiveMaterialModel::GetFillingSolid() {
    return filling_solid_;
}

double PassiveMaterialModel::GetMaxSize() {
    return max_size_;
}
