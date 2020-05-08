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
#include "tools/geant4.h"

#include "DetectorConstructionG4.hpp"
#include "Parameterization2DG4.hpp"

using namespace allpix;

GeometryConstructionG4::GeometryConstructionG4(GeometryManager* geo_manager, Configuration& config)
    : geo_manager_(geo_manager), config_(config) {
    passive_builder_ = new PassiveMaterialConstructionG4(geo_manager_);
    passive_builder_->registerVolumes();
}

/**
 * @brief Version of std::make_shared that does not delete the pointer
 *
 * This version is needed because some pointers are deleted by Geant4 internally, but they are stored as std::shared_ptr in
 * the framework.
 */
template <typename T, typename... Args> static std::shared_ptr<T> make_shared_no_delete(Args... args) {
    return std::shared_ptr<T>(new T(args...), [](T*) {});
}

/**
 * First initializes all the materials. Then constructs the world from the internally calculated world size with a certain
 * margin. Finally builds all the individual detectors.
 */

G4VPhysicalVolume* GeometryConstructionG4::Construct() {
    // Initialize materials
    init_materials();

    // Set world material
    auto world_material = config_.get<std::string>("world_material", "air");
    std::transform(world_material.begin(), world_material.end(), world_material.begin(), ::tolower);
    if(materials_.find(world_material) == materials_.end()) {
        throw InvalidValueError(config_, "world_material", "material does not exists, use 'air' or 'vacuum'");
    }

    world_material_ = materials_[world_material];
    // Add world_material to the list of materials to be called from other modules
    materials_["world_material"] = world_material_;
    LOG(TRACE) << "Material of world is " << world_material_->GetName();

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
    world_log_ = std::make_shared<G4LogicalVolume>(world_box.get(), world_material_, "World_log", nullptr, nullptr, nullptr);

    // Set the world to invisible in the viewer
    world_log_->SetVisAttributes(G4VisAttributes::GetInvisible());

    // Place the world at the center
    world_phys_ =
        std::make_unique<G4PVPlacement>(nullptr, G4ThreeVector(0., 0., 0.), world_log_.get(), "World", nullptr, false, 0);

    // Build all the geometries that have been added to the GeometryBuilder vector, including Detectors and Target
    passive_builder_->buildVolumes(materials_, world_log_);
    const auto& detBuilder = new DetectorConstructionG4(geo_manager_);
    detBuilder->build(materials_, world_log_);

    // Check for overlaps:
    check_overlaps();

    return world_phys_.get();
}

/**
 * Initializes all the internal materials. The following materials are supported by this module:
 * - vacuum
 * - air
 * - silicon
 * - epoxy
 * - kapton
 * - solder
 * - lithium
 * - beryllium
 */
void GeometryConstructionG4::init_materials() {
    G4NistManager* nistman = G4NistManager::Instance();

    // Build table of materials from database
    materials_["silicon"] = nistman->FindOrBuildMaterial("G4_Si");
    materials_["plexiglass"] = nistman->FindOrBuildMaterial("G4_PLEXIGLASS");
    materials_["kapton"] = nistman->FindOrBuildMaterial("G4_KAPTON");
    materials_["copper"] = nistman->FindOrBuildMaterial("G4_Cu");
    materials_["aluminum"] = nistman->FindOrBuildMaterial("G4_Al");
    materials_["air"] = nistman->FindOrBuildMaterial("G4_AIR");
    materials_["lead"] = nistman->FindOrBuildMaterial("G4_Pb");
    materials_["tungsten"] = nistman->FindOrBuildMaterial("G4_W");
    materials_["lithium"] = nistman->FindOrBuildMaterial("G4_Li");
    materials_["beryllium"] = nistman->FindOrBuildMaterial("G4_Be");

    // Create required elements:
    G4Element* H = new G4Element("Hydrogen", "H", 1., 1.01 * CLHEP::g / CLHEP::mole);
    G4Element* C = new G4Element("Carbon", "C", 6., 12.01 * CLHEP::g / CLHEP::mole);
    G4Element* O = new G4Element("Oxygen", "O", 8., 16.0 * CLHEP::g / CLHEP::mole);
    G4Element* Cl = new G4Element("Chlorine", "Cl", 17., 35.45 * CLHEP::g / CLHEP::mole);
    G4Element* Sn = new G4Element("Tin", "Sn", 50., 118.710 * CLHEP::g / CLHEP::mole);
    G4Element* Pb = new G4Element("Lead", "Pb", 82., 207.2 * CLHEP::g / CLHEP::mole);

    // Add vacuum
    materials_["vacuum"] = new G4Material("Vacuum", 1, 1.008 * CLHEP::g / CLHEP::mole, CLHEP::universe_mean_density);

    // Create Epoxy material
    G4Material* Epoxy = new G4Material("Epoxy", 1.3 * CLHEP::g / CLHEP::cm3, 3);
    Epoxy->AddElement(H, 44);
    Epoxy->AddElement(C, 15);
    Epoxy->AddElement(O, 7);
    materials_["epoxy"] = Epoxy;

    // Create Carbon Fiber material:
    G4Material* CarbonFiber = new G4Material("CarbonFiber", 1.5 * CLHEP::g / CLHEP::cm3, 2);
    CarbonFiber->AddMaterial(Epoxy, 0.4);
    CarbonFiber->AddElement(C, 0.6);
    materials_["carbonfiber"] = CarbonFiber;

    // Create PCB G-10 material
    G4Material* GTen = new G4Material("G10", 1.7 * CLHEP::g / CLHEP::cm3, 3);
    GTen->AddMaterial(nistman->FindOrBuildMaterial("G4_SILICON_DIOXIDE"), 0.773);
    GTen->AddMaterial(Epoxy, 0.147);
    GTen->AddElement(Cl, 0.08);
    materials_["g10"] = GTen;

    // Create solder material
    G4Material* Solder = new G4Material("Solder", 8.4 * CLHEP::g / CLHEP::cm3, 2);
    Solder->AddElement(Sn, 0.63);
    Solder->AddElement(Pb, 0.37);
    materials_["solder"] = Solder;
}

void GeometryConstructionG4::check_overlaps() {
    G4PhysicalVolumeStore* phys_volume_store = G4PhysicalVolumeStore::GetInstance();
    LOG(TRACE) << "Checking overlaps";
    bool overlapFlag = false;
    // Release Geant4 output for better error description
    RELEASE_STREAM(G4cout);
    for(auto volume : (*phys_volume_store)) {
        overlapFlag = volume->CheckOverlaps(1000, 0., false) || overlapFlag;
    }
    // Supress again to prevent further complications
    SUPPRESS_STREAM(G4cout);
    if(overlapFlag) {
        LOG(ERROR) << "Overlapping volumes detected.";
    } else {
        LOG(INFO) << "No overlapping volumes detected.";
    }
}
