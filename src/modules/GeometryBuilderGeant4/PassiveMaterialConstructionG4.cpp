/**
 * @file
 * @brief Wrapper for the Geant4 Passive Material Construction
 * @remarks Code is based on code from Mathieu Benoit
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
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

#include <G4LogicalVolume.hh>
#include <G4LogicalVolumeStore.hh>
#include <G4PVPlacement.hh>
#include <G4RotationMatrix.hh>
#include <G4ThreeVector.hh>
#include <G4VisAttributes.hh>

#include "PassiveMaterialVolume.hpp"
#include "core/module/exceptions.h"
#include "tools/ROOT.h"
#include "tools/geant4.h"

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

PassiveMaterialConstructionG4::PassiveMaterialConstructionG4(GeometryManager* geo_manager) : geo_manager_(geo_manager) {}

void PassiveMaterialConstructionG4::registerVolumes() {
    std::list<Configuration>& passive_configs = geo_manager_->getPassiveElements();
    LOG(TRACE) << "Building " << passive_configs.size() << " passive material volume(s)";

    for(auto& passive_config : passive_configs) {
        if(std::find(passive_volume_names_.begin(), passive_volume_names_.end(), passive_config.getName()) !=
           passive_volume_names_.end()) {
            throw ModuleError("Pasive Material '" + passive_config.getName() + "' is defined more than once.");
            continue;
        }
        auto passive_volume = new PassiveMaterialVolume(passive_config, geo_manager_);
        passive_volumes_.push_back(passive_volume);
        passive_volume_names_.push_back(passive_config.getName());
    }
}

void PassiveMaterialConstructionG4::buildVolumes(const std::map<std::string, G4Material*>& materials) {
    for(auto& passive_volume : passive_volumes_) {
        passive_volume->buildVolume(materials);
    }
}
