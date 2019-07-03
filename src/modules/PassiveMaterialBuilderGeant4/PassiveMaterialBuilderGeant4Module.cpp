/**
 * @file
 * @brief Implementation of Geant4 geometry construction module
 * @copyright Copyright (c) 2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "PassiveMaterialBuilderGeant4Module.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <utility>

#include <G4RunManager.hh>
#include <G4UImanager.hh>
#include <G4UIterminal.hh>
#include <G4Version.hh>
#include <G4VisManager.hh>

#include <Math/Vector3D.h>

#include "tools/ROOT.h"
#include "tools/geant4.h"

#include "core/config/ConfigReader.hpp"
#include "core/config/exceptions.h"
#include "core/geometry/GeometryManager.hpp"

#include "PassiveMaterialConstructionG4.hpp"
#include "core/config/ConfigReader.hpp"
#include "core/utils/log.h"

using namespace allpix;
using namespace ROOT;

PassiveMaterialBuilderGeant4Module::PassiveMaterialBuilderGeant4Module(Configuration& config,
                                                                       Messenger*,
                                                                       GeometryManager* geo_manager)
    : Module(config), geo_manager_(geo_manager), run_manager_g4_(nullptr) {}

/**
 * @brief Checks if a particular Geant4 dataset is available in the environment
 * @throws ModuleError If a certain Geant4 dataset is not set or not available
 */
void PassiveMaterialBuilderGeant4Module::init() {

    // Suppress all output (also stdout due to a part in Geant4 where G4cout is not used)
    SUPPRESS_STREAM(std::cout);
    SUPPRESS_STREAM(G4cout);

    // Create the G4 run manager
    run_manager_g4_ = G4RunManager::GetRunManager();

    // Release stdout again
    RELEASE_STREAM(std::cout);

    ConfigManager* conf_manager = getConfigManager();
    for(auto& passive_material_section : conf_manager->getPassiveMaterialConfigurations()) {
        std::shared_ptr<PassiveMaterialConstructionG4> passive_material_builder =
            std::make_shared<PassiveMaterialConstructionG4>(passive_material_section);

        geo_manager_->addBuilder(passive_material_builder);
    }

    // Add the max and min points of the passive material to the world volume
    // auto points = new PassiveMaterialConstructionG4(passive_material_section);
    // points_ = points->addPoints();
    // for(auto& point : points_) {
    // geo_manager_->addPoint(point);
    // }

    // Run the geometry construct function in GeometryConstructionG4
    // LOG(TRACE) << "Building Geant4 geometry";
    // run_manager_g4_->InitializeGeometry();

    // Release output from G4
    RELEASE_STREAM(G4cout);
}
