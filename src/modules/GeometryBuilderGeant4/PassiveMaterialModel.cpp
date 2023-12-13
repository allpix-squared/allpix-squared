/**
 * @file
 * @brief Implements the construction of passive material volumes
 * @remarks Code is based on code from Mathieu Benoit
 *
 * @copyright Copyright (c) 2019-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <string>

#include <G4LogicalVolume.hh>
#include <G4LogicalVolumeStore.hh>
#include <G4Material.hh>
#include <G4PVPlacement.hh>
#include <G4RotationMatrix.hh>
#include <G4ThreeVector.hh>
#include <G4VisAttributes.hh>

#include "core/module/exceptions.h"
#include "tools/ROOT.h"
#include "tools/geant4/geant4.h"

#include "MaterialManager.hpp"
#include "PassiveMaterialModel.hpp"
#include "passive_models/BoxModel.hpp"
#include "passive_models/CylinderModel.hpp"
#include "passive_models/GDMLModel.hpp"
#include "passive_models/SphereModel.hpp"

using namespace allpix;
using namespace ROOT::Math;

std::shared_ptr<PassiveMaterialModel> allpix::PassiveMaterialModel::factory(const Configuration& config,
                                                                            GeometryManager* geo_manager) {
    auto type = config.get<std::string>("type");
    if(type == "box") {
        return std::make_shared<BoxModel>(config, geo_manager);
    } else if(type == "cylinder") {
        return std::make_shared<CylinderModel>(config, geo_manager);
    } else if(type == "sphere") {
        return std::make_shared<SphereModel>(config, geo_manager);
    } else if(type == "gdml") {
#ifdef Geant4_GDML
        return std::make_shared<GDMLModel>(config, geo_manager);
#else
        throw allpix::InvalidValueError(
            config,
            "type",
            "GDML not supported by Geant4 version. Recompile Geant4 with the option -DGEANT4_USE_GDML=ON to enable support");
#endif
    } else {
        throw ModuleError("Passive Material has an unknown type " + type);
    }
}

PassiveMaterialModel::PassiveMaterialModel(const Configuration& config, GeometryManager* geo_manager)
    : config_(config), geo_manager_(geo_manager), name_(config.getName()) {

    mother_volume_ = config_.get<std::string>("mother_volume", "");

    LOG(DEBUG) << "Registering volume: " << getName();
    LOG(DEBUG) << " Mother volume: " << getMotherVolume();
    // Get the orientation and position of the material
    orientation_ = geo_manager_->getPassiveElementOrientation(getName()).second;

    position_ = geo_manager_->getPassiveElementOrientation(getName()).first;
    std::vector<double> copy_vec(9);
    orientation_.GetComponents(copy_vec.begin(), copy_vec.end());
    XYZPoint vx, vy, vz;
    orientation_.GetComponents(vx, vy, vz);
    rotation_ = std::make_shared<G4RotationMatrix>(copy_vec.data());

    LOG(DEBUG) << "Registered volume.";
}

void PassiveMaterialModel::buildVolume(const std::shared_ptr<G4LogicalVolume>& world_log) {

    LOG(TRACE) << "Building passive material: " << getName();
    G4LogicalVolume* mother_log_volume = nullptr;
    if(!getMotherVolume().empty()) {
        G4LogicalVolumeStore* log_volume_store = G4LogicalVolumeStore::GetInstance();
        mother_log_volume = log_volume_store->GetVolume(getMotherVolume() + "_log");
    } else {
        mother_log_volume = world_log.get();
    }

    if(mother_log_volume == nullptr) {
        throw InvalidValueError(config_, "mother_volume", "mother_volume does not exist");
    }

    G4ThreeVector position_vector = toG4Vector(position_);
    G4Transform3D transform_phys(*rotation_, position_vector);

    auto& materials = Materials::getInstance();
    auto material = config_.get<std::string>("material");
    G4Material* g4material = nullptr;
    try {
        g4material = materials.get(material);
    } catch(ModuleError& e) {
        throw InvalidValueError(config_, "material", e.what());
    }

    LOG(TRACE) << "Creating Geant4 model for '" << getName() << "' of type '" << config_.get<std::string>("type") << "'";
    LOG(TRACE) << " -Material\t\t:\t " << material << "( " << g4material->GetName() << " )";
    LOG(TRACE) << " -Position\t\t:\t " << Units::display(position_, {"mm", "um"});

    // Get the solid from the Model
    auto solid = get_solid();

    // Place the logical volume of the passive material
    auto log_volume = make_shared_no_delete<G4LogicalVolume>(solid.get(), g4material, getName() + "_log");
    geo_manager_->setExternalObject(getName(), "passive_material_log", log_volume);

    // Set VisAttribute of the material
    set_visualization_attributes(log_volume.get(), mother_log_volume);

    // Place the physical volume of the passive material
    auto phys_volume = make_shared_no_delete<G4PVPlacement>(
        transform_phys, log_volume.get(), getName() + "_phys", mother_log_volume, false, 0, true);
    geo_manager_->setExternalObject(getName(), "passive_material_phys", phys_volume);

    auto unused_keys = config_.getUnusedKeys();
    if(!unused_keys.empty()) {
        std::stringstream st;
        st << "Unused configuration keys in passive material definition:";
        for(auto& key : unused_keys) {
            st << std::endl << key;
        }
        LOG(WARNING) << st.str();
    }

    LOG(TRACE) << " Constructed passive material " << getName() << " successfully";
}

void PassiveMaterialModel::set_visualization_attributes(G4LogicalVolume* volume, G4LogicalVolume* mother_volume) {

    auto& materials = Materials::getInstance();

    auto pm_color = config_.get<ROOT::Math::XYZPoint>("color", ROOT::Math::XYZPoint());
    auto opacity = config_.get<double>("opacity", 0.4);
    if(pm_color != ROOT::Math::XYZPoint()) {
        auto* pm_vol_col = new G4VisAttributes(G4Colour(pm_color.x(), pm_color.y(), pm_color.z(), opacity));
        volume->SetVisAttributes(pm_vol_col);
    }
    // Set VisAttribute to invisible if material is equal to the material of its mother volume
    else if(volume->GetMaterial() == mother_volume->GetMaterial()) {
        LOG(WARNING) << "Material of passive material " << getName()
                     << " is the same as the material of its mother volume! Material will not be shown in the simulation.";
        volume->SetVisAttributes(G4VisAttributes::GetInvisible());
    }
    // Set VisAttribute to white if material = world_material
    else if(volume->GetMaterial() == materials.get("world_material")) {
        auto* white_vol = new G4VisAttributes(G4Colour(1.0, 1.0, 1.0, 0.4));
        volume->SetVisAttributes(white_vol);
    }
}

void PassiveMaterialModel::add_points() {
    std::array<int, 8> offset_x = {{1, 1, 1, 1, -1, -1, -1, -1}};
    std::array<int, 8> offset_y = {{1, 1, -1, -1, 1, 1, -1, -1}};
    std::array<int, 8> offset_z = {{1, -1, 1, -1, 1, -1, 1, -1}};
    // Add the min and max points for every type
    auto max_size = getMaxSize();
    if(max_size == 0) {
        throw ModuleError("Passive Material '" + getName() +
                          "' does not have a maximum size parameter associated with its model");
    }
    for(size_t i = 0; i < 8; ++i) {
        auto points_vector =
            G4ThreeVector(offset_x.at(i) * max_size / 2, offset_y.at(i) * max_size / 2, offset_z.at(i) * max_size / 2);
        // Rotate the outer points of the material
        points_vector *= *rotation_;
        points_vector += toG4Vector(position_);
        LOG(TRACE) << "adding point " << Units::display(points_vector, {"mm", "um"}) << "to the geometry";
        geo_manager_->addPoint(ROOT::Math::XYZPoint(points_vector));
    }
}
