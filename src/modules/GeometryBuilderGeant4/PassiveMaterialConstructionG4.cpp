/**
 * @file
 * @brief Wrapper for the Geant4 Passive Material Construction
 * @remarks Code is based on code from Mathieu Benoit
 *
 * @copyright Copyright (c) 2019-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "PassiveMaterialConstructionG4.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

#include <G4LogicalVolume.hh>

#include "PassiveMaterialModel.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/module/exceptions.h"
#include "core/utils/log.h"

using namespace allpix;

PassiveMaterialConstructionG4::PassiveMaterialConstructionG4(GeometryManager* geo_manager) : geo_manager_(geo_manager) {}

void PassiveMaterialConstructionG4::registerVolumes() {
    auto passive_configs = geo_manager_->getPassiveElements();
    LOG(TRACE) << "Registering " << passive_configs.size() << " passive material volume(s)";

    for(auto& passive_config : passive_configs) {
        std::shared_ptr<PassiveMaterialModel> const model = PassiveMaterialModel::factory(passive_config, geo_manager_);
        passive_volumes_.emplace_back(model);
    }

    // Sort the volumes according to their hierarchy (mother volumes before dependants)
    std::function<int(
        const std::shared_ptr<PassiveMaterialModel>&, const std::vector<std::shared_ptr<PassiveMaterialModel>>&, size_t)>
        hierarchy;
    hierarchy = [&hierarchy](const std::shared_ptr<PassiveMaterialModel>& vol,
                             const std::vector<std::shared_ptr<PassiveMaterialModel>>& vols,
                             size_t n) {
        auto m = std::find_if(vols.begin(), vols.end(), [&vol](const std::shared_ptr<PassiveMaterialModel>& v) {
            return v->getName() == vol->getMotherVolume();
        });

        if(n > 100) {
            throw ModuleError(
                "Hierarchy of mother volumes cannot be resolved. The configuration might hold circular dependencies.");
        }
        return (m == vols.end() ? 0 : hierarchy(*m, vols, ++n)) + 1;
    };

    std::sort(passive_volumes_.begin(),
              passive_volumes_.end(),
              [&hierarchy, volumes = passive_volumes_](const std::shared_ptr<PassiveMaterialModel>& lhs,
                                                       const std::shared_ptr<PassiveMaterialModel>& rhs) {
                  return (hierarchy(lhs, volumes, 0) < hierarchy(rhs, volumes, 0));
              });
}

void PassiveMaterialConstructionG4::buildVolumes(const std::shared_ptr<G4LogicalVolume>& world_log) {
    for(auto& passive_volume : passive_volumes_) {
        passive_volume->buildVolume(world_log);
    }
}
