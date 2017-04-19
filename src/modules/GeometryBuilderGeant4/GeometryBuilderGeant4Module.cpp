#include "GeometryBuilderGeant4Module.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <utility>

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

#include "core/config/ConfigReader.hpp"
#include "core/config/exceptions.h"
#include "core/geometry/GeometryManager.hpp"
#include "core/utils/log.h"

// temporary common includes
#include "modules/common/DetectorModelG4.hpp"
#include "modules/common/ReadGeoDescription.hpp"

using namespace allpix;
using namespace ROOT;

// constructor and destructor
GeometryBuilderGeant4Module::GeometryBuilderGeant4Module(Configuration config, Messenger*, GeometryManager* geo_manager)
    : config_(std::move(config)), geo_manager_(geo_manager), run_manager_g4_(nullptr) {
    // construct the internal geometry
    // WARNING: we need to do this here to allow for proper instantiation (initialization is only run after loading)
    // FIXME: move this to a separate module or the core?

    LOG(INFO) << "Constructing internal geometry";
    // read the models
    std::vector<std::string> model_paths;
    if(config_.has("model_paths")) {
        model_paths = config_.getPathArray("model_paths", true);
    }
    auto geo_descriptions = ReadGeoDescription(model_paths);

    // construct the detectors from the config file
    std::string detector_file_name = config_.getPath("detectors_file", true);
    std::ifstream file(detector_file_name);
    ConfigReader detector_config(file, detector_file_name);

    // add the configurations to the detectors
    for(auto& detector_section : detector_config.getConfigurations()) {
        std::shared_ptr<DetectorModel> detector_model =
            geo_descriptions.getDetectorModel(detector_section.get<std::string>("type"));

        // check if detector model is defined
        if(detector_model == nullptr) {
            throw InvalidValueError(detector_section, "type", "detector type does not exist in registered models");
        }

        // get the position and orientation
        auto position = detector_section.get<Math::XYZPoint>("position", Math::XYZPoint());
        auto orientation = detector_section.get<Math::EulerAngles>("orientation", Math::EulerAngles());

        // create the detector and add it
        auto detector = std::make_shared<Detector>(detector_section.getName(), detector_model, position, orientation);
        geo_manager_->addDetector(detector);
    }
}
GeometryBuilderGeant4Module::~GeometryBuilderGeant4Module() = default;

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
    bool simple_view = config_.get<bool>("simple_view", false);

    // set the geometry constructor
    GeometryConstructionG4* geometry_construction = new GeometryConstructionG4(geo_manager_, world_size, simple_view);
    run_manager_g4_->SetUserInitialization(geometry_construction);

    // run the geometry construct function in GeometryConstructionG4
    LOG(INFO) << "Building Geant4 geometry";
    run_manager_g4_->InitializeGeometry();

    // release output from G4
    RELEASE_STREAM(G4cout);
}
