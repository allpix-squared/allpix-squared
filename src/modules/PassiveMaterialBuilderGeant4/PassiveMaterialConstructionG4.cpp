/**
 * @file
 * @brief Implements the Geant4 geometry construction process
 * @remarks Code is based on code from Mathieu Benoit
 * @copyright Copyright (c) 2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "PassiveMaterialConstructionG4.hpp"
#include <memory>
#include <string>
#include <utility>

#include <Math/RotationX.h>
#include <Math/RotationY.h>
#include <Math/RotationZ.h>
#include <Math/RotationZYX.h>
#include <Math/Vector3D.h>

#include <G4Box.hh>
#include <G4LogicalVolume.hh>
#include <G4LogicalVolumeStore.hh>
#include <G4Material.hh>
#include <G4NistManager.hh>
#include <G4PVDivision.hh>
#include <G4PVPlacement.hh>
#include <G4PhysicalVolumeStore.hh>
#include <G4Sphere.hh>
#include <G4StepLimiterPhysics.hh>
#include <G4SubtractionSolid.hh>
#include <G4ThreeVector.hh>
#include <G4Tubs.hh>
#include <G4UnionSolid.hh>
#include <G4UserLimits.hh>
#include <G4VSolid.hh>
#include <G4VisAttributes.hh>
#include "core/module/exceptions.h"
#include "core/utils/log.h"
#include "tools/ROOT.h"
#include "tools/geant4.h"

using namespace allpix;

PassiveMaterialConstructionG4::PassiveMaterialConstructionG4(Configuration& config) : config_(config) {}

/**
 * @brief Version of std::make_shared that does not delete the pointer
 *
 * This version is needed because some pointers are deleted by Geant4 internally, but they are stored as std::shared_ptr in
 * the framework.
 */
template <typename T, typename... Args> static std::shared_ptr<T> make_shared_no_delete(Args... args) {
    return std::shared_ptr<T>(new T(args...), [](T*) {});
}

void PassiveMaterialConstructionG4::build(G4LogicalVolume* world_log, std::map<std::string, G4Material*> materials_) {
    /*
    Get the name of the Passive Material
    */
    std::string name = config_.getName();
    /*
    Get the world_material
    */
    auto world_material = world_log->GetMaterial()->GetName();

    /*
    Get the information for the passive materials
    */
    ROOT::Math::XYVector passive_material_size = config_.get<ROOT::Math::XYVector>("passive_material_size", {0, 0});
    auto passive_material_thickness = config_.get<double>("passive_material_thickness", 0);

    ROOT::Math::XYZPoint passive_material_location =
        config_.get<ROOT::Math::XYZPoint>("passive_material_location", {0., 0., 0.});

    std::string passive_material = config_.get<std::string>("passive_material", world_material);
    std::transform(passive_material.begin(), passive_material.end(), passive_material.begin(), ::tolower);

    auto passive_material_box = std::make_shared<G4Box>(
        "passive_material_", passive_material_size.x(), passive_material_size.y(), passive_material_thickness);
    solids_.push_back(passive_material_box);
    auto passive_material_log = make_shared_no_delete<G4LogicalVolume>(
        passive_material_box.get(), materials_[passive_material], "passive_material_log");

    // Place the passive_material box
    auto passive_material_pos = toG4Vector(passive_material_location);
    auto passive_material_phys_ = make_shared_no_delete<G4PVPlacement>(
        nullptr, passive_material_pos, passive_material_log.get(), name + "_phys", world_log, false, 0, true);
}
