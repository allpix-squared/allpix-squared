/**
 * @file
 * @brief Implementation of Geant4 geometry construction module
 * @copyright Copyright (c) 2017-2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "PassiveMaterialBuilderGeant4Module.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <utility>

#include <Math/Vector3D.h>

#include "tools/ROOT.h"
#include "tools/geant4.h"

#include "core/config/ConfigReader.hpp"
#include "core/config/exceptions.h"
#include "core/geometry/GeometryManager.hpp"

#include "PassiveMaterialConstructionG4.hpp"
#include "core/utils/log.h"

using namespace allpix;
using namespace ROOT;

PassiveMaterialBuilderGeant4Module::PassiveMaterialBuilderGeant4Module(Configuration& config,
                                                                       Messenger*,
                                                                       GeometryManager* geo_manager)
    : Module(config), geo_manager_(geo_manager) {}

void PassiveMaterialBuilderGeant4Module::init() {

    // Suppress output from G4
    SUPPRESS_STREAM(G4cout);
    ConfigManager* conf_manager = getConfigManager();
    LOG(TRACE) << "Building " << conf_manager->getPassiveMaterialConfigurations().size() << " passive material(s)";

    for(auto& passive_material_section : conf_manager->getPassiveMaterialConfigurations()) {
        std::shared_ptr<PassiveMaterialConstructionG4> passive_material_builder =
            std::make_shared<PassiveMaterialConstructionG4>(passive_material_section);

        // Add the passive materials to objects that will be built in GeometryBuilderGeant4 Module
        geo_manager_->addBuilder(passive_material_builder);
        // Get the min and max points for the passive materials
        points_ = passive_material_builder->addPoints();
    }
    // Add the max and min points to the world volume
    for(auto& point : points_) {
        LOG(TRACE) << "adding point " << Units::display(point, {"mm", "um"}) << "to the geometry";
        geo_manager_->addPoint(point);
    }

    // Release output from G4
    RELEASE_STREAM(G4cout);
}
