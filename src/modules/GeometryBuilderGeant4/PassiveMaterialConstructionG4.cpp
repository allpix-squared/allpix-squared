/**
 * @file
 * @brief Implements the Geant4 passive material construction process
 * @remarks Code is based on code from Mathieu Benoit
 * @copyright Copyright (c) 2017-2019 CERN and the Allpix Squared authors.
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
#include <G4IntersectionSolid.hh>
#include <G4Sphere.hh>
#include <G4SubtractionSolid.hh>
#include <G4Tubs.hh>
#include <G4UnionSolid.hh>

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
PassiveMaterialConstructionG4::PassiveMaterialConstructionG4(Configuration& config, GeometryManager* geo_manager)
    : config_(config), geo_manager_(geo_manager) {
    name_ = config_.getName();
    passive_material_type_ = config_.get<std::string>("type");
    if(passive_material_type_ == "box") {
        model_ = std::make_shared<BoxModel>(config_);
    } else if(passive_material_type_ == "cylinder") {
        model_ = std::make_shared<CylinderModel>(config_);
    } else if(passive_material_type_ == "sphere") {
        model_ = std::make_shared<SphereModel>(config_);
    } else {
        throw ModuleError("Pasive Material '" + name_ + "' has an incorrect type.");
    }
    // Get the orientation and position of the material
    auto orientation = geo_manager->getOrientation(config_);

    position_ = orientation.first;
    std::vector<double> copy_vec(9);
    orientation.second.GetComponents(copy_vec.begin(), copy_vec.end());
    XYZPoint vx, vy, vz;
    orientation.second.GetComponents(vx, vy, vz);
    rotWrapper = std::make_shared<G4RotationMatrix>(copy_vec.data());
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

void PassiveMaterialConstructionG4::build(std::map<std::string, G4Material*> materials_) {
    /*
    Get the information for the passive materials
    */
    auto mother_volume = config_.get<std::string>("mother_volume", "World").append("_log");
    G4LogicalVolumeStore* log_volume_store = G4LogicalVolumeStore::GetInstance();
    mother_log_volume_ = log_volume_store->GetVolume(mother_volume);
    if(mother_log_volume_ == nullptr) {
        throw InvalidValueError(config_, "mother_volume", "mother_volume does not exist");
    }

    G4ThreeVector posWrapper = toG4Vector(position_);
    G4Transform3D transform_phys(*rotWrapper, posWrapper);

    auto passive_material = config_.get<std::string>("material");
    std::transform(passive_material.begin(), passive_material.end(), passive_material.begin(), ::tolower);

    LOG(TRACE) << "Creating Geant4 model for '" << name_ << "' of type '" << passive_material_type_ << "'";
    LOG(TRACE) << " -Material\t\t:\t " << passive_material << "( " << materials_[passive_material]->GetName() << " )";
    LOG(TRACE) << " -Position\t\t:\t " << Units::display(position_, {"mm", "um"});

    // Get the solid from the Model
    auto solid = std::shared_ptr<G4VSolid>(model_->getSolid());
    if(solid == nullptr) {
        throw ModuleError("Pasive Material '" + name_ + "' does not have a solid associated with its model");
    }
    solids_.push_back(solid);

    // Place the logical volume of the passive material
    auto log_volume = make_shared_no_delete<G4LogicalVolume>(solid.get(), materials_[passive_material], name_ + "_log");
    geo_manager_->setExternalObject("passive_material_log", log_volume, name_);

    // Set VisAttribute to invisible if material is equal to the material of its mother volume
    if(materials_[passive_material] == mother_log_volume_->GetMaterial()) {
        LOG(WARNING) << "Material of passive material " << name_
                     << " is the same as the material of its mother volume! Material will not be shown in the simulation.";
        log_volume->SetVisAttributes(G4VisAttributes::GetInvisible());
    }
    // Set VisAttribute to white if material = world_material
    else if(materials_[passive_material] == materials_["world_material"]) {
        auto white_vol = new G4VisAttributes(G4Colour(1.0, 1.0, 1.0, 0.4));
        log_volume->SetVisAttributes(white_vol);
    }

    // Place the physical volume of the passive material
    auto phys_volume = make_shared_no_delete<G4PVPlacement>(
        transform_phys, log_volume.get(), name_ + "_phys", mother_log_volume_, false, 0, true);
    geo_manager_->setExternalObject("passive_material_phys", phys_volume, name_);
    LOG(TRACE) << " Constructed passive material " << name_ << " successfully";
}

std::vector<XYZPoint> PassiveMaterialConstructionG4::addPoints() {
    std::array<int, 8> offset_x = {{1, 1, 1, 1, -1, -1, -1, -1}};
    std::array<int, 8> offset_y = {{1, 1, -1, -1, 1, 1, -1, -1}};
    std::array<int, 8> offset_z = {{1, -1, 1, -1, 1, -1, 1, -1}};
    // Add the min and max points for every type
    auto max_size = model_->getMaxSize();
    if(max_size == 0) {
        throw ModuleError("Pasive Material '" + name_ +
                          "' does not have a maximum size parameter associated with its model");
    }
    for(size_t i = 0; i < 8; ++i) {
        auto points_vector =
            G4ThreeVector(offset_x.at(i) * max_size / 2, offset_y.at(i) * max_size / 2, offset_z.at(i) * max_size / 2);
        // Rotate the outer points of the material
        points_vector *= *rotWrapper;
        points_.emplace_back(XYZPoint(
            position_.x() + points_vector.x(), position_.y() + points_vector.y(), position_.z() + points_vector.z()));
    }
    return points_;
}
