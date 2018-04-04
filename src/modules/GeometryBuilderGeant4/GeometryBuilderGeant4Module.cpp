/**
 * @file
 * @brief Implementation of Geant4 geometry construction module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "GeometryBuilderGeant4Module.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <utility>

#include <G4RunManager.hh>
#include <G4UImanager.hh>
#include <G4UIterminal.hh>
#include <G4VisManager.hh>

#include <Math/Vector3D.h>

#include "GeometryConstructionG4.hpp"

#include "tools/ROOT.h"
#include "tools/geant4.h"

#include "core/config/ConfigReader.hpp"
#include "core/config/exceptions.h"
#include "core/geometry/GeometryManager.hpp"
#include "core/utils/log.h"

// Include GDML if Geant4 version has it
#ifdef Geant4_GDML
#include "G4GDMLParser.hh"
#endif

using namespace allpix;
using namespace ROOT;

GeometryBuilderGeant4Module::GeometryBuilderGeant4Module(Configuration& config, Messenger*, GeometryManager* geo_manager)
    : Module(config), geo_manager_(geo_manager), run_manager_g4_(nullptr) {}

/**
 * @brief Checks if a particular Geant4 dataset is available in the environment
 * @throws ModuleError If a certain Geant4 dataset is not set or not available
 */
static void check_dataset_g4(const std::string& env_name) {
    const char* file_name = std::getenv(env_name.c_str());
    if(file_name == nullptr) {
        /* clang-format off */
        throw ModuleError("Geant4 environment variable " + env_name +
                          " is not set, make sure to source a Geant4 "
                          "environment with all datasets");
        /* clang-format on */
    }
    std::ifstream file(file_name);
    if(!file.good()) {
        /* clang-format off */
        throw ModuleError("Geant4 environment variable " + env_name +
                          " does not point to existing dataset, the Geant4 "
                          "environment is invalid");
        /* clang-format on */
    }
    // FIXME: check if file does actually contain a correct dataset
}

void GeometryBuilderGeant4Module::init() {
    // Check if all the required geant4 datasets are defined
    LOG(DEBUG) << "Checking Geant4 datasets";
    check_dataset_g4("G4LEVELGAMMADATA");
    check_dataset_g4("G4RADIOACTIVEDATA");
    check_dataset_g4("G4PIIDATA");
    check_dataset_g4("G4SAIDXSDATA");
    check_dataset_g4("G4ABLADATA");
    check_dataset_g4("G4REALSURFACEDATA");
    check_dataset_g4("G4NEUTRONHPDATA");
    check_dataset_g4("G4NEUTRONXSDATA");
    check_dataset_g4("G4ENSDFSTATEDATA");
    check_dataset_g4("G4LEDATA");

    // Suppress all output (also stdout due to a part in Geant4 where G4cout is not used)
    SUPPRESS_STREAM(std::cout);
    SUPPRESS_STREAM(G4cout);

    // Create the G4 run manager
    run_manager_g4_ = std::make_unique<G4RunManager>();

    // Release stdout again
    RELEASE_STREAM(std::cout);

    // Set the geometry construction to use
    auto geometry_construction = new GeometryConstructionG4(geo_manager_, config_);
    run_manager_g4_->SetUserInitialization(geometry_construction);

    // Run the geometry construct function in GeometryConstructionG4
    LOG(TRACE) << "Building Geant4 geometry";
    run_manager_g4_->InitializeGeometry();

    // Export geometry in GDML if requested (and GDML support is available in Geant4)
    if(config_.has("GDML_output_file")) {
#ifdef Geant4_GDML
        std::string GDML_output_file = createOutputFile(config_.get<std::string>("GDML_output_file"));
        if(GDML_output_file.size() <= 5 || GDML_output_file.substr(GDML_output_file.size() - 5, 5) != ".gdml") {
            GDML_output_file += ".gdml";
        }
        G4GDMLParser parser;
        parser.SetRegionExport(true);
        parser.Write(GDML_output_file,
                     G4TransportationManager::GetTransportationManager()
                         ->GetNavigatorForTracking()
                         ->GetWorldVolume()
                         ->GetLogicalVolume());
#else
        std::string error = "You requested to export the geometry in GDML. ";
        error += "However, GDML support is currently disabled in Geant4. ";
        error += "To enable it, configure and compile Geant4 with the option -DGEANT4_USE_GDML=ON.";
        throw allpix::InvalidValueError(config_, "GDML_output_file", error);
#endif
    }

    // Release output from G4
    RELEASE_STREAM(G4cout);
}
