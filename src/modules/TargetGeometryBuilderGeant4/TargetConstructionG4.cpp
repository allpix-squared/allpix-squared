/**
 * @file
 * @brief Implements the Geant4 geometry construction process
 * @remarks Code is based on code from Mathieu Benoit
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "TargetConstructionG4.hpp"
#include <memory>
#include <string>
#include <utility>

#include <G4Box.hh>
#include <G4LogicalVolume.hh>
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

#include "core/geometry/HybridPixelDetectorModel.hpp"
#include "core/module/exceptions.h"
#include "core/utils/log.h"
#include "tools/ROOT.h"
#include "tools/geant4.h"

using namespace allpix;

TargetConstructionG4::TargetConstructionG4(Configuration& config) : config_(config) {}

/**
 * @brief Version of std::make_shared that does not delete the pointer
 *
 * This version is needed because some pointers are deleted by Geant4 internally, but they are stored as std::shared_ptr in
 * the framework.
 */
template <typename T, typename... Args> static std::shared_ptr<T> make_shared_no_delete(Args... args) {
    return std::shared_ptr<T>(new T(args...), [](T*) {});
}

void TargetConstructionG4::build(void* world, void* materials) {

    /*
    Reinterpret the void* world and void* materials to make them fit as G4LogicalVolume and std::map<std::string,
    G4Material*>
    */

    G4LogicalVolume* world_log = reinterpret_cast<G4LogicalVolume*>(world);
    std::map<std::string, G4Material*>* materials_ = reinterpret_cast<std::map<std::string, G4Material*>*>(materials);

    std::string world_material = config_.get<std::string>("world_material", "air");

    /*
    Get all the required variables for the target from the config file
    */

    ROOT::Math::XYVector target_size = config_.get<ROOT::Math::XYVector>("target_size", {0, 0});
    double target_thickness = config_.get<double>("target_thickness", 0);

    ROOT::Math::XYZPoint target_location = config_.get<ROOT::Math::XYZPoint>("target_location", {0., 0., 0.});

    std::string target_material = config_.get<std::string>("target_material", world_material);
    std::transform(target_material.begin(), target_material.end(), target_material.begin(), ::tolower);

    auto target_box = std::make_shared<G4Box>("target_", target_size.x(), target_size.y(), target_thickness);
    solids_.push_back(target_box);
    auto target_log = make_shared_no_delete<G4LogicalVolume>(target_box.get(), (*materials_)[target_material], "target_log");

    // Place the target box
    auto target_pos = toG4Vector(target_location);
    auto target_phys_ = make_shared_no_delete<G4PVPlacement>(
        nullptr, target_pos, target_log.get(), "sensor_phys", world_log, false, 0, true);
}
