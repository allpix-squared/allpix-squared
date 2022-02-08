/**
 * @file
 * @brief Implements the Geant4 geometry construction process
 * @remarks Code is based on code from Mathieu Benoit
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "GeometryConstructionG4.hpp"
#include "MaterialManager.hpp"

#include <memory>
#include <string>
#include <utility>

#include <G4Box.hh>
#include <G4LogicalVolume.hh>
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
#include "tools/geant4/G4LoggingDestination.hpp"
#include "tools/geant4/geant4.h"

#include "Parameterization2DG4.hpp"

using namespace allpix;

GeometryConstructionG4::GeometryConstructionG4(GeometryManager* geo_manager, Configuration& config)
    : geo_manager_(geo_manager), config_(config) {
    detector_builder_ = std::make_unique<DetectorConstructionG4>(geo_manager_);
    passive_builder_ = std::make_unique<PassiveMaterialConstructionG4>(geo_manager_);
    passive_builder_->registerVolumes();
}

/**
 * First initializes all the materials. Then constructs the world from the internally calculated world size with a certain
 * margin. Finally builds all the individual detectors.
 */
G4VPhysicalVolume* GeometryConstructionG4::Construct() {
    // Initialize materials
    auto& materials = Materials::getInstance();

    // Set world material
    G4Material* g4material_world = nullptr;
    auto world_material = config_.get<std::string>("world_material", "air");
    try {
        g4material_world = materials.get(world_material);
    } catch(ModuleError& e) {
        throw InvalidValueError(config_, "world_material", e.what());
    }

    // Register the world material for others as reference:
    materials.set("world_material", g4material_world);
    LOG(TRACE) << "Material of world is " << g4material_world->GetName();

    // Calculate world size
    ROOT::Math::XYZVector half_world_size;
    ROOT::Math::XYZPoint min_coord = geo_manager_->getMinimumCoordinate();
    ROOT::Math::XYZPoint max_coord = geo_manager_->getMaximumCoordinate();
    half_world_size.SetX(std::max(std::abs(min_coord.x()), std::abs(max_coord.x())));
    half_world_size.SetY(std::max(std::abs(min_coord.y()), std::abs(max_coord.y())));
    half_world_size.SetZ(std::max(std::abs(min_coord.z()), std::abs(max_coord.z())));

    // Calculate and apply margins to world size
    auto margin_percentage = config_.get<double>("world_margin_percentage", 0.1);
    auto minimum_margin = config_.get<ROOT::Math::XYZPoint>("world_minimum_margin", {0, 0, 0});
    double add_x = half_world_size.x() * margin_percentage;
    if(add_x < minimum_margin.x()) {
        add_x = minimum_margin.x();
    }
    double add_y = half_world_size.y() * margin_percentage;
    if(add_y < minimum_margin.y()) {
        add_y = minimum_margin.y();
    }
    double add_z = half_world_size.z() * margin_percentage;
    if(add_z < minimum_margin.z()) {
        add_z = minimum_margin.z();
    }
    half_world_size.SetX(half_world_size.x() + add_x);
    half_world_size.SetY(half_world_size.y() + add_y);
    half_world_size.SetZ(half_world_size.z() + add_z);

    LOG(DEBUG) << "World size is " << Units::display(2 * half_world_size, {"mm"});

    // Build the world
    auto world_box = std::make_shared<G4Box>("World", half_world_size.x(), half_world_size.y(), half_world_size.z());
    solids_.push_back(world_box);
    world_log_ =
        std::make_shared<G4LogicalVolume>(world_box.get(), g4material_world, "World_log", nullptr, nullptr, nullptr);

    // Set the world to invisible in the viewer
    world_log_->SetVisAttributes(G4VisAttributes::GetInvisible());
    geo_manager_->setExternalObject("", "world_log", world_log_);

    // Place the world at the center
    world_phys_ =
        std::make_unique<G4PVPlacement>(nullptr, G4ThreeVector(0., 0., 0.), world_log_.get(), "World", nullptr, false, 0);

    // Build all the geometries that have been added to the GeometryBuilder vector, including Detectors and Target
    passive_builder_->buildVolumes(world_log_);
    detector_builder_->build(world_log_);

    // Check for overlaps:
    check_overlaps();

    return world_phys_.get();
}

void GeometryConstructionG4::check_overlaps() {
    G4PhysicalVolumeStore* phys_volume_store = G4PhysicalVolumeStore::GetInstance();
    LOG(TRACE) << "Checking overlaps";
    bool overlapFlag = false;

    auto current_level = G4LoggingDestination::getG4coutReportingLevel();
    G4LoggingDestination::setG4coutReportingLevel(LogLevel::ERROR);
    for(auto* volume : (*phys_volume_store)) {
        overlapFlag = volume->CheckOverlaps(1000, 0., false) || overlapFlag;
    }
    G4LoggingDestination::setG4coutReportingLevel(current_level);

    if(overlapFlag) {
        LOG(ERROR) << "Overlapping volumes detected.";
    } else {
        LOG(INFO) << "No overlapping volumes detected.";
    }
}
