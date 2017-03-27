#include "GeometryBuilderGeant4Module.hpp"

#include <cassert>
#include <cstdio>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <G4PhysListFactory.hh>
#include <G4RunManager.hh>

#include <G4UImanager.hh>
#include <G4UIterminal.hh>
#include <G4VisExecutive.hh>
#include <G4VisManager.hh>

#include <Math/EulerAngles.h>
#include <Math/Vector3D.h>

#include "GeometryConstructionG4.hpp"

#include "tools/ROOT.h"
#include "tools/geant4.h"

#include "core/AllPix.hpp"
#include "core/config/ConfigReader.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/utils/exceptions.h"
#include "core/utils/log.h"

// temporary common includes
#include "modules/common/DetectorModelG4.hpp"
#include "modules/common/ReadGeoDescription.hpp"

using namespace allpix;
using namespace ROOT;

// name of the module
const std::string GeometryBuilderGeant4Module::name = "GeometryBuilderGeant4";

// constructor and destructor (defined here to allow for incomplete unique_ptr type)
GeometryBuilderGeant4Module::GeometryBuilderGeant4Module(Configuration config, Messenger*, GeometryManager* geo_manager)
    : config_(std::move(config)), geo_manager_(geo_manager), run_manager_g4_(nullptr) {}
GeometryBuilderGeant4Module::~GeometryBuilderGeant4Module() = default;

// check geant4 environment variable
inline static void check_dataset_g4(const std::string& env_name) {
    const char* file_name = std::getenv(env_name.c_str());
    if(file_name == nullptr) {
        throw ModuleException("Geant4 environment variable " + env_name + " is not set, make sure to source a Geant4 "
                                                                          "environment with all datasets");
    }
    std::ifstream file(file_name);
    if(!file.good()) {
        throw ModuleException("Geant4 environment variable " + env_name + " does not point to existing dataset, your Geant4 "
                                                                          "environment is not complete");
    }
    // FIXME: check if file does actually contain a correct dataset
}

// create the run manager and make it available
void GeometryBuilderGeant4Module::init() {
    // suppress all output (also cout due to a part in Geant4 where G4cout is not used)
    SUPPRESS_STREAM(std::cout);
    SUPPRESS_STREAM(G4cout);

    // create the G4 run manager
    run_manager_g4_ = std::make_unique<G4RunManager>();

    // check if all the required geant4 datasets are defined
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

    // release the output again
    RELEASE_STREAM(std::cout);
    RELEASE_STREAM(G4cout);

    // construct the geometry
    // WARNING: we need to do this here to allow for proper instantiation later (FIXME: is this correct)

    // read the models
    std::string model_file_name = config_.get<std::string>("models_file");
    auto geo_descriptions = ReadGeoDescription(model_file_name);

    // construct the detectors from the config file
    std::string detector_file_name = config_.get<std::string>("detectors_file");
    std::ifstream file(detector_file_name);
    if(!file) {
        throw allpix::ConfigFileUnavailableError(detector_file_name);
    }
    ConfigReader detector_config(file);

    // add the configurations to the detectors
    for(auto& detector_section : detector_config.getConfigurations()) {
        std::shared_ptr<DetectorModel> detector_model =
            geo_descriptions.getDetectorModel(detector_section.get<std::string>("type"));

        if(detector_model == nullptr) {
            throw InvalidValueError("type",
                                    detector_section.getName(),
                                    detector_section.getText("type"),
                                    "detector type does not exist in registered models");
        }

        Math::XYZVector position = detector_section.get<Math::XYZVector>("position", Math::XYZVector());
        Math::EulerAngles orientation = detector_section.get<Math::EulerAngles>("orientation", Math::EulerAngles());

        auto detector = std::make_shared<Detector>(detector_section.getName(), detector_model, position, orientation);
        geo_manager_->addDetector(detector);
    }

    // save the geant4 run manager in allpix to make it available to other modules
    // getAllPix()->setExternalManager(run_manager_g4_);
}

// run the geometry construction
void GeometryBuilderGeant4Module::run() {
    LOG(INFO) << "START BUILD GEOMETRY";

    // FIXME: check that geometry is empty or clean it before continuing
    assert(run_manager_g4_ != nullptr);

    // construct the G4 geometry
    build_g4();

    // finish
    LOG(INFO) << "END BUILD GEOMETRY";
}

void GeometryBuilderGeant4Module::build_g4() {
    // suppress all output for G4
    SUPPRESS_STREAM(G4cout);

    // get the world size
    config_.setDefault("world_size", G4ThreeVector(1000, 1000, 2000));
    G4ThreeVector world_size = config_.get<G4ThreeVector>("world_size");

    // set the geometry constructor
    GeometryConstructionG4* geometry_construction = new GeometryConstructionG4(geo_manager_, world_size);
    run_manager_g4_->SetUserInitialization(geometry_construction);

    // set the physics list
    // FIXME: set a good default physics list
    config_.setDefault("physics_list", "QGSP_BERT");
    G4PhysListFactory physListFactory;
    G4VUserPhysicsList* physicsList = physListFactory.GetReferencePhysList(config_.get<std::string>("physics_list"));
    if(physicsList == nullptr) {
        // FIXME: better syntax for exceptions here
        // FIXME: more information about available lists
        throw InvalidValueError(
            "physics_list", config_.getName(), config_.getText("physics_list"), "physics list is not defined");
    }
    run_manager_g4_->SetUserInitialization(physicsList);

    // run the construct function in GeometryConstructionG4
    run_manager_g4_->Initialize();

    // release output from G4
    RELEASE_STREAM(G4cout);
}
