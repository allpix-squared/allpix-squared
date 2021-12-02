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

// Include GDML if Geant4 version has it
#ifdef Geant4_GDML
#include "G4GDMLParser.hh"
#endif

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

    // Load gdml file
    if(config_.has("GDML_input_file")) {
        import_gdml();
    }

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

void GeometryConstructionG4::import_gdml() {
#ifdef Geant4_GDML
    std::vector<std::string> GDML_input_files = config_.getPathArray("GDML_input_file");

    // Initialize offset positions at origin
    std::vector<std::vector<double>> GDML_input_offsets(GDML_input_files.size(), std::vector<double>(3, 0));

    // Set the offset values
    if(config_.has("GDML_input_offset")) {
        GDML_input_offsets = config_.getMatrix<double>("GDML_input_offset");
        if(GDML_input_files.size() != GDML_input_offsets.size()) {
            throw allpix::InvalidValueError(config_,
                                            "GDML_input_offset",
                                            "If GDML offsets are specified, number "
                                            "of values has to be consistent with the "
                                            "number of specified models.");
        }
        for(auto row : GDML_input_offsets) {
            if(row.size() != 3) {
                throw allpix::InvalidValueError(config_, "GDML_input_offset", "GDML offsets need to be three dimensional.");
            }
        }
    }

    // Loop over all GDML input files
    int idx = 0;
    std::vector<std::string> name_list; // Contains the names of the daughter volumes

    for(auto GDML_input_file : GDML_input_files) {
        std::vector<double> offset = GDML_input_offsets.at(static_cast<long unsigned int>(idx));
        G4ThreeVector GDML_input_offset = G4ThreeVector(offset[0], offset[1], offset[2]);
        idx++;

        G4GDMLParser parser;
        parser.Read(GDML_input_file, false);
        G4VPhysicalVolume* gdml_phys = parser.GetWorldVolume();

        G4LogicalVolume* gdml_log = gdml_phys->GetLogicalVolume();
        if(gdml_log->GetName() == "World") {
            std::string error = "The geometry you requested to import in GDML";
            error += "contains a World Volume with the name \"World\" which is colliding";
            error += "with the one of the framework. Please rename it in order to "
                     "proceed.";
            throw allpix::InvalidValueError(config_, "GDML_input_file", error);
        }

        int gdml_no_daughters = gdml_log->GetNoDaughters();
        LOG(DEBUG) << "Number of daughter volumes " << gdml_no_daughters;
        if(gdml_no_daughters != 0) {
            // gdml_phys->SetTranslation(gdml_phys->GetTranslation() +
            // GDML_input_offset);
            for(int i = 0; i < gdml_no_daughters; i++) {
                G4VPhysicalVolume* gdml_daughter = gdml_log->GetDaughter(i);
                G4LogicalVolume* gdml_daughter_log = gdml_daughter->GetLogicalVolume();

                // Remove the daughter from its world volume in order to add it to the
                // global one
                gdml_log->RemoveDaughter(gdml_daughter);

                std::string gdml_daughter_name = gdml_daughter->GetName();
                if(!name_list.empty() &&
                   std::find(name_list.begin(), name_list.end(), gdml_daughter_name) != name_list.end()) {
                    gdml_daughter_name += "_";
                    gdml_daughter->SetName(gdml_daughter_name);
                    gdml_daughter->SetCopyNo(gdml_daughter->GetCopyNo() + 1);
                    gdml_daughter_log->SetName(gdml_daughter_name);
                }

                LOG(DEBUG) << "Volume " << i << ": " << gdml_daughter_name;
                name_list.push_back(gdml_daughter_name);

                // Add offset to current daughter location
                gdml_daughter->SetTranslation(gdml_daughter->GetTranslation() + GDML_input_offset);

                // Get auxiliary information
                G4GDMLAuxListType aux_info = parser.GetVolumeAuxiliaryInformation(gdml_daughter_log);

                // Check if color information is available and set it to the daughter
                // volume
                for(auto aux : aux_info) {
                    std::string str = aux.type;
                    std::string val = aux.value;
                    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
                    if(str == "color" || str == "colour") {
                        G4Colour color = get_color(val);
                        gdml_daughter_log->SetVisAttributes(G4VisAttributes(color));
                    }
                }

                // Add the physical daughter volume to the world volume
                world_log_.get()->AddDaughter(gdml_daughter);

                // Set new mother volume to the global one
                gdml_daughter->SetMotherLogical(world_log_.get());
            }
        } else {
            LOG(DEBUG) << "Add daughter";
            gdml_phys->SetTranslation(GDML_input_offset);
            LOG(DEBUG) << "Volume " << gdml_phys->GetName();
            world_log_.get()->AddDaughter(gdml_phys);
        }
    }

#else
    std::string error = "You requested to import the geometry in GDML. ";
    error += "However, GDML support is currently disabled in Geant4. ";
    error += "To enable it, configure and compile Geant4 with the option "
             "-DGEANT4_USE_GDML=ON.";
    throw allpix::InvalidValueError(config_, "GDML_input_file", error);
#endif
}

G4Colour GeometryConstructionG4::get_color(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);

    int r, g, b, a;
    r = g = b = a = 256;
    if(value.size() >= 6) {
        // Value contains RGBA color
        value.erase(std::remove(value.begin(), value.end(), '#'), value.end());
        std::istringstream(value.substr(0, 2)) >> std::hex >> r;
        std::istringstream(value.substr(2, 2)) >> std::hex >> g;
        std::istringstream(value.substr(4, 2)) >> std::hex >> b;
        if(value.size() >= 8) {
            std::istringstream(value.substr(6, 2)) >> std::hex >> a;
        }
    }

    // If no valid color code was specified, return white
    return G4Colour(static_cast<double>(r) / 256,
                    static_cast<double>(g) / 256,
                    static_cast<double>(b) / 256,
                    static_cast<double>(a) / 256);
}
