/**
 * @file
 * @brief Implements the construction of passive material volumes
 * @remarks Code is based on code from Mathieu Benoit
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <memory>
#include <string>
#include <utility>

#include <Math/RotationX.h>
#include <Math/RotationY.h>
#include <Math/RotationZ.h>
#include <Math/RotationZYX.h>
#include <Math/Vector3D.h>

#include <G4Box.hh>
#include <G4IntersectionSolid.hh>
#include <G4Sphere.hh>
#include <G4SubtractionSolid.hh>
#include <G4Tubs.hh>
#include <G4UnionSolid.hh>
#include "G4Material.hh"

#include <G4LogicalVolume.hh>
#include <G4LogicalVolumeStore.hh>
#include <G4PVPlacement.hh>
#include <G4RotationMatrix.hh>
#include <G4ThreeVector.hh>
#include <G4VisAttributes.hh>

#include "core/module/exceptions.h"
#include "tools/ROOT.h"
#include "tools/geant4.h"

#include "PassiveMaterialModel.hpp"
#include "Passive_Material_Models/BoxModel.hpp"
#include "Passive_Material_Models/CylinderModel.hpp"
#include "Passive_Material_Models/SphereModel.hpp"

using namespace allpix;
using namespace ROOT::Math;

/**
 * @brief Version of std::make_shared that does not delete the pointer
 *
 * This version is needed because some pointers are deleted by Geant4 internally, but they are stored as std::shared_ptr in
 * the framework.
 */
template <typename T, typename... Args> static std::shared_ptr<T> make_shared_no_delete(Args... args) {
    return std::shared_ptr<T>(new T(args...), [](T*) {});
}

std::shared_ptr<PassiveMaterialModel>
allpix::PassiveMaterialModel::Factory(std::string type, Configuration config, GeometryManager* geo_manager) {
    if(type == "box") {
        return std::make_shared<BoxModel>(config, geo_manager);
    } else if(type == "cylinder") {
        return std::make_shared<CylinderModel>(config, geo_manager);
    } else if(type == "sphere") {
        return std::make_shared<SphereModel>(config, geo_manager);
    } else {
        throw ModuleError("Passive Material has an unknown type " + type);
    }
}

PassiveMaterialModel::PassiveMaterialModel(Configuration config, GeometryManager* geo_manager)
    : config_(config), geo_manager_(geo_manager) {

    name_ = config_.getName();
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

void PassiveMaterialModel::buildVolume(const std::map<std::string, G4Material*>& materials,
                                       const std::shared_ptr<G4LogicalVolume>& world_log) {

    LOG(TRACE) << "Building passive material: " << getName();
    G4LogicalVolume* mother_log_volume = nullptr;
    if(!getMotherVolume().empty()) {
        G4LogicalVolumeStore* log_volume_store = G4LogicalVolumeStore::GetInstance();
        mother_log_volume = log_volume_store->GetVolume(getMotherVolume().append("_log"));
    } else {
        mother_log_volume = world_log.get();
    }

    if(mother_log_volume == nullptr) {
        throw InvalidValueError(config_, "mother_volume", "mother_volume does not exist");
    }

    G4ThreeVector position_vector = toG4Vector(position_);
    G4Transform3D transform_phys(*rotation_, position_vector);

    auto passive_material = config_.get<std::string>("material");
    std::transform(passive_material.begin(), passive_material.end(), passive_material.begin(), ::tolower);

    LOG(TRACE) << "Creating Geant4 model for '" << getName() << "' of type '" << config_.get<std::string>("type") << "'";
    LOG(TRACE) << " -Material\t\t:\t " << passive_material << "( " << materials.at(passive_material)->GetName() << " )";
    LOG(TRACE) << " -Position\t\t:\t " << Units::display(position_, {"mm", "um"});

    // Get the solid from the Model
    auto solid = std::shared_ptr<G4VSolid>(getSolid());
    if(solid == nullptr) {
        throw ModuleError("Passive Material '" + getName() + "' does not have a solid associated with its model");
    }
    solids_.push_back(solid);

    // Place the logical volume of the passive material
    auto log_volume =
        make_shared_no_delete<G4LogicalVolume>(solid.get(), materials.at(passive_material), getName() + "_log");
    geo_manager_->setExternalObject(getName(), "passive_material_log", log_volume);

    // Set VisAttribute to invisible if material is equal to the material of its mother volume
    if(materials.at(passive_material) == mother_log_volume->GetMaterial()) {
        LOG(WARNING) << "Material of passive material " << getName()
                     << " is the same as the material of its mother volume! Material will not be shown in the simulation.";
        log_volume->SetVisAttributes(G4VisAttributes::GetInvisible());
    }
    // Set VisAttribute to white if material = world_material
    else if(materials.at(passive_material) == materials.at("world_material")) {
        auto white_vol = new G4VisAttributes(G4Colour(1.0, 1.0, 1.0, 0.4));
        log_volume->SetVisAttributes(white_vol);
    }

    // Place the physical volume of the passive material
    auto phys_volume = make_shared_no_delete<G4PVPlacement>(
        transform_phys, log_volume.get(), getName() + "_phys", mother_log_volume, false, 0, true);
    geo_manager_->setExternalObject(getName(), "passive_material_phys", phys_volume);
    LOG(TRACE) << " Constructed passive material " << getName() << " successfully";
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
