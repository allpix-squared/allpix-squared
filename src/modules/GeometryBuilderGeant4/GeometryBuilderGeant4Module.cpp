/**
 * @file
 * @brief Implementation of Geant4 geometry construction module
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "GeometryBuilderGeant4Module.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <utility>

#include <G4UImanager.hh>
#include <G4UIterminal.hh>
#include <G4Version.hh>
#include <G4VisManager.hh>

#include <Math/Vector3D.h>

#include "DetectorConstructionG4.hpp"
#include "GeometryConstructionG4.hpp"
#include "PassiveMaterialConstructionG4.hpp"

#include "tools/ROOT.h"
#include "tools/geant4/geant4.h"

#include "core/config/ConfigReader.hpp"
#include "core/config/exceptions.h"
#include "core/geometry/GeometryManager.hpp"
#include "core/utils/log.h"
#include "tools/geant4/G4LoggingDestination.hpp"
#include "tools/geant4/MTRunManager.hpp"
#include "tools/geant4/RunManager.hpp"

using namespace allpix;
using namespace ROOT;

GeometryBuilderGeant4Module::GeometryBuilderGeant4Module(Configuration& config, Messenger*, GeometryManager* geo_manager)
    : Module(config), geo_manager_(geo_manager), run_manager_g4_(nullptr) {
    geometry_construction_ = new GeometryConstructionG4(geo_manager_, config_);
    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();

    // Read Geant4 verbosity configuration
    auto g4cerr_log_level = config_.get<std::string>("log_level_g4cerr", "WARNING");
    std::transform(g4cerr_log_level.begin(), g4cerr_log_level.end(), g4cerr_log_level.begin(), ::toupper);
    auto g4cout_log_level = config_.get<std::string>("log_level_g4cout", "TRACE");
    std::transform(g4cout_log_level.begin(), g4cout_log_level.end(), g4cout_log_level.begin(), ::toupper);

    // Set Geant4 G4cerr log level
    try {
        LogLevel log_level = Log::getLevelFromString(g4cerr_log_level);
        G4LoggingDestination::setG4cerrReportingLevel(log_level);
    } catch(std::invalid_argument& e) {
        throw InvalidValueError(config_, "geant4_cerr_log_level", "invalid log level provided");
    }

    // Set Geant G4cout log level
    try {
        LogLevel log_level = Log::getLevelFromString(g4cout_log_level);
        G4LoggingDestination::setG4coutReportingLevel(log_level);
    } catch(std::invalid_argument& e) {
        throw InvalidValueError(config_, "geant4_cout_log_level", "invalid log level provided");
    }
}

/**
 * @brief Checks if a particular Geant4 dataset is available in the environment
 * @throws ModuleError If a certain Geant4 dataset is not set or not available
 */
static void check_dataset_g4(const std::string& env_name) {
    const char* file_name = std::getenv(env_name.c_str());
    if(file_name == nullptr) {
        throw ModuleError("Geant4 environment variable " + env_name +
                          " is not set, make sure to source a Geant4 "
                          "environment with all datasets");
    }
    std::ifstream file(file_name);
    if(!file.good()) {
        throw ModuleError("Geant4 environment variable " + env_name +
                          " does not point to existing dataset, the Geant4 "
                          "environment is invalid");
    }
    // FIXME: check if file does actually contain a correct dataset
}

void GeometryBuilderGeant4Module::initialize() {
    // Check if all the required geant4 datasets are defined
    LOG(DEBUG) << "Checking Geant4 datasets";
    check_dataset_g4("G4LEVELGAMMADATA");
    check_dataset_g4("G4RADIOACTIVEDATA");
    check_dataset_g4("G4PIIDATA");
    check_dataset_g4("G4SAIDXSDATA");
    check_dataset_g4("G4ABLADATA");
    check_dataset_g4("G4REALSURFACEDATA");
    check_dataset_g4("G4NEUTRONHPDATA");
    check_dataset_g4("G4ENSDFSTATEDATA");
    check_dataset_g4("G4LEDATA");

// Check for Neutron XS data only for Geant4 version prior to 10.5, deprecated dataset from 10.5
#if G4VERSION_NUMBER < 1050
    check_dataset_g4("G4NEUTRONXSDATA");
#endif

    // Suppress all stdout output due to a part in Geant4 where G4cout is not used
    SUPPRESS_STREAM(std::cout);

    G4UImanager* ui_g4 = G4UImanager::GetUIpointer();
    ui_g4->SetCoutDestination(G4LoggingDestination::getInstance());

    // Create the G4 run manager. If multithreading was requested we use the custom run manager
    // that support calling BeamOn operations in parallel. Otherwise we use default manager.
    if(multithreadingEnabled()) {
        run_manager_g4_ = std::make_unique<MTRunManager>();
    } else {
        run_manager_g4_ = std::make_unique<RunManager>();
    }

    // Release stdout again
    RELEASE_STREAM(std::cout);

    // Set the geometry construction to use
    run_manager_g4_->SetUserInitialization(geometry_construction_);

    // Run the geometry construct function in GeometryConstructionG4
    LOG(TRACE) << "Building Geant4 geometry";
    run_manager_g4_->InitializeGeometry();
}
