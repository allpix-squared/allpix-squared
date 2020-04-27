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
PassiveMaterialConstructionG4::PassiveMaterialConstructionG4(GeometryManager* geo_manager) : geo_manager_(geo_manager) {}

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
    std::list<Configuration>& passive_configs = geo_manager_->getPassiveElements();

    for(auto& passive_config : passive_configs) {
        auto name = passive_config.getName();
        auto passive_material_type = passive_config.get<std::string>("type");
        std::shared_ptr<PassiveMaterialModel> model;
        if(passive_material_type == "box") {
            model = std::make_shared<BoxModel>(passive_config);
        } else if(passive_material_type == "cylinder") {
            model = std::make_shared<CylinderModel>(passive_config);
        } else if(passive_material_type == "sphere") {
            model = std::make_shared<SphereModel>(passive_config);
        } else {
            throw ModuleError("Pasive Material '" + name + "' has an incorrect type.");
        }
        // Get the orientation and position of the material
        auto orientation = geo_manager_->getOrientation(passive_config);

        auto position = orientation.first;
        std::vector<double> copy_vec(9);
        orientation.second.GetComponents(copy_vec.begin(), copy_vec.end());
        XYZPoint vx, vy, vz;
        orientation.second.GetComponents(vx, vy, vz);
        auto rotWrapper = std::make_shared<G4RotationMatrix>(copy_vec.data());

        // FIXME!!! Get correct mother volume
        auto mother_volume = passive_config.get<std::string>("mother_volume", "World").append("_log");
        G4LogicalVolumeStore* log_volume_store = G4LogicalVolumeStore::GetInstance();
        G4LogicalVolume* mother_log_volume = log_volume_store->GetVolume(mother_volume);
        if(mother_log_volume == nullptr) {
            throw InvalidValueError(passive_config, "mother_volume", "mother_volume does not exist");
        }

        LOG(DEBUG) << "Building pasive material: " << passive_config.getName();

        G4ThreeVector posWrapper = toG4Vector(position);
        G4Transform3D transform_phys(*rotWrapper, posWrapper);

        LOG(DEBUG) << "Check for material";

        auto passive_material = passive_config.get<std::string>("material");
        std::transform(passive_material.begin(), passive_material.end(), passive_material.begin(), ::tolower);

        LOG(TRACE) << "Creating Geant4 model for '" << name << "' of type '" << passive_material_type << "'";
        LOG(TRACE) << " -Material\t\t:\t " << passive_material << "( " << materials_[passive_material]->GetName() << " )";
        LOG(TRACE) << " -Position\t\t:\t " << Units::display(position, {"mm", "um"});

        // Get the solid from the Model
        auto solid = std::shared_ptr<G4VSolid>(model->getSolid());
        if(solid == nullptr) {
            throw ModuleError("Pasive Material '" + name + "' does not have a solid associated with its model");
        }
        solids_.push_back(solid);

        // Place the logical volume of the passive material
        auto log_volume = make_shared_no_delete<G4LogicalVolume>(solid.get(), materials_[passive_material], name + "_log");
        geo_manager_->setExternalObject("passive_material_log", log_volume, name);

        // Set VisAttribute to invisible if material is equal to the material of its mother volume
        if(materials_[passive_material] == mother_log_volume->GetMaterial()) {
            LOG(WARNING)
                << "Material of passive material " << name
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
            transform_phys, log_volume.get(), name + "_phys", mother_log_volume, false, 0, true);
        geo_manager_->setExternalObject("passive_material_phys", phys_volume, name);
        LOG(TRACE) << " Constructed passive material " << name << " successfully";
    }
}
