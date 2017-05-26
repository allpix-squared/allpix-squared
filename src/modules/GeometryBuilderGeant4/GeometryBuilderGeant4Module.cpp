#include "GeometryBuilderGeant4Module.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <utility>

#include <G4RunManager.hh>
#include <G4UImanager.hh>
#include <G4UIterminal.hh>
#include <G4VisManager.hh>

#include <Math/EulerAngles.h>
#include <Math/Vector3D.h>

#include "GeometryConstructionG4.hpp"

#include "tools/ROOT.h"
#include "tools/geant4.h"

#include "core/config/ConfigReader.hpp"
#include "core/config/exceptions.h"
#include "core/geometry/GeometryManager.hpp"
#include "core/utils/log.h"

// temporary common includes
#include "modules/common/DetectorModelG4.hpp"

// GDML
#ifdef Geant4_GDML
#include "G4GDMLParser.hh"
#endif

using namespace allpix;
using namespace ROOT;

// constructor and destructor
GeometryBuilderGeant4Module::GeometryBuilderGeant4Module(Configuration config, Messenger*, GeometryManager* geo_manager)
    : Module(config), config_(std::move(config)), geo_manager_(geo_manager), run_manager_g4_(nullptr) {}

// check geant4 environment variable
inline static void check_dataset_g4(const std::string& env_name) {
    const char* file_name = std::getenv(env_name.c_str());
    if(file_name == nullptr) {
        throw ModuleError("Geant4 environment variable " + env_name + " is not set, make sure to source a Geant4 "
                                                                      "environment with all datasets");
    }
    std::ifstream file(file_name);
    if(!file.good()) {
        throw ModuleError("Geant4 environment variable " + env_name + " does not point to existing dataset, your Geant4 "
                                                                      "environment is not complete");
    }
    // FIXME: check if file does actually contain a correct dataset
}

// create the run manager, check Geant4 state and construct the Geant4 geometry
void GeometryBuilderGeant4Module::init() {
    // suppress all output (also cout due to a part in Geant4 where G4cout is not used)
    SUPPRESS_STREAM(std::cout);
    SUPPRESS_STREAM(G4cout);

    // create the G4 run manager
    run_manager_g4_ = std::make_unique<G4RunManager>();

    // release stdout again
    RELEASE_STREAM(std::cout);

    // check if all the required geant4 datasets are defined
    LOG(DEBUG) << "checking Geant4 datasets";
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

    // get the world size
    config_.setDefault("world_size", G4ThreeVector(1000, 1000, 2000));
    G4ThreeVector world_size = config_.get<G4ThreeVector>("world_size");
    auto simple_view = config_.get<bool>("simple_view", false);

    // set the geometry constructor
    auto geometry_construction = new GeometryConstructionG4(geo_manager_, world_size, simple_view);
    run_manager_g4_->SetUserInitialization(geometry_construction);

    // run the geometry construct function in GeometryConstructionG4
    LOG(TRACE) << "Building Geant4 geometry";
    run_manager_g4_->InitializeGeometry();

    // export geometry in GDML.
    if(config_.has("GDML_output_file")) {
#ifdef Geant4_GDML
        std::string GDML_output_file = getOutputPath(config_.get<std::string>("GDML_output_file"));
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

    // release output from G4
    RELEASE_STREAM(G4cout);
}
