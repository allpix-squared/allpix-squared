/**
 * @file
 * @brief Implementation of Geant4 passive material construction module
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

#include "PassiveMaterialConstructionG4.hpp"
#include "core/config/ConfigReader.hpp"
#include "core/geometry/GeometryManager.hpp"

using namespace allpix;

PassiveMaterialBuilderGeant4Module::PassiveMaterialBuilderGeant4Module(Configuration& config,
                                                                       Messenger*,
                                                                       GeometryManager* geo_manager)
    : Module(config), geo_manager_(geo_manager) {}

void PassiveMaterialBuilderGeant4Module::init() {

    // Suppress output from G4
    SUPPRESS_STREAM(G4cout);

    ConfigManager* conf_manager = getConfigManager();
    std::set<std::string> passive_material_names;

    LOG(TRACE) << "Adding " << conf_manager->getPassiveMaterialConfigurations().size()
               << " passive material(s) to the list of builders.";

    for(auto& passive_material_section : conf_manager->getPassiveMaterialConfigurations()) {
        auto name = passive_material_section.getName();
        if(passive_material_names.find(name) != passive_material_names.end()) {
            throw ModuleError("Passive Material with name '" + name +
                              "' is already registered, Passive Material names should be unique");
        }
        passive_material_names.insert(name);

        std::shared_ptr<PassiveMaterialConstructionG4> passive_material_builder =
            std::make_shared<PassiveMaterialConstructionG4>(passive_material_section, geo_manager_);

        // Add the passive materials to objects that will be built in GeometryBuilderGeant4 Module
        geo_manager_->addBuilder(passive_material_builder);

        // Add the min and max points to the world volume
        for(auto& point : passive_material_builder->addPoints()) {
            LOG(TRACE) << "adding point " << Units::display(point, {"mm", "um"}) << "to the geometry";
            geo_manager_->addPoint(point);
        }
    }

    // Release output from G4
    RELEASE_STREAM(G4cout);
}
