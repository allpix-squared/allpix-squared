/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 * @author Mathieu Benoit <benoit@lal.in2p3.fr>
 */

#include "DepositionGeant4Module.hpp"

#include <limits>
#include <string>
#include <utility>

#include <G4HadronicProcessStore.hh>
#include <G4PhysListFactory.hh>
#include <G4RunManager.hh>
#include <G4StepLimiterPhysics.hh>
#include <G4UImanager.hh>
#include <G4UserLimits.hh>

#include "core/config/InvalidValueError.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/module/exceptions.h"
#include "core/utils/log.h"
#include "tools/geant4.h"

#include "GeneratorActionG4.hpp"
#include "SensitiveDetectorActionG4.hpp"

// FIXME: broken common includes
#include "modules/common/DetectorModelG4.hpp"

using namespace allpix;

const std::string DepositionGeant4Module::name = "DepositionGeant4";

DepositionGeant4Module::DepositionGeant4Module(Configuration config, Messenger* messenger, GeometryManager* geo_manager)
    : config_(std::move(config)), messenger_(messenger), geo_manager_(geo_manager), user_limits_() {
    // create user limits for maximum step length in the sensor
    user_limits_ =
        std::make_unique<G4UserLimits>(config_.get<double>("max_step_length", std::numeric_limits<double>::max()));
}
DepositionGeant4Module::~DepositionGeant4Module() = default;

void DepositionGeant4Module::init() {
    // load the G4 run manager (which is currently owned by the geometry builder...)
    run_manager_g4_ = G4RunManager::GetRunManager();
    if(run_manager_g4_ == nullptr) {
        throw ModuleError("Cannot deposit charges using Geant4 without a Geant4 geometry builder");
    }

    // suppress all output for G4
    SUPPRESS_STREAM(G4cout);

    // find the physics list
    // FIXME: this is not on the correct place in the event / run logic
    // FIXME: set a good default physics list
    G4PhysListFactory physListFactory;
    G4VModularPhysicsList* physicsList = physListFactory.GetReferencePhysList(config_.get<std::string>("physics_list"));
    if(physicsList == nullptr) {
        // FIXME: more information about available lists
        throw InvalidValueError(config_, "physics_list", "specified physics list does not exists");
    }
    // use a step limiter in a later stage
    physicsList->RegisterPhysics(new G4StepLimiterPhysics());
    // initialize the physics list
    LOG(INFO) << "Initializing physics processes";
    run_manager_g4_->SetUserInitialization(physicsList);
    run_manager_g4_->InitializePhysics();

    // initialize the full run manager to ensure correct state flags
    run_manager_g4_->Initialize();

    // build generator
    LOG(INFO) << "Constructing particle source";
    GeneratorActionG4* generator = new GeneratorActionG4(config_);
    run_manager_g4_->SetUserAction(generator);

    // get the creation energy for charge (default is silicon electron hole pair energy)
    // FIXME: is this a good name...
    double charge_creation_energy = config_.get<double>("charge_creation_energy", 3.64e-6);

    // loop through all detectors and set the sensitive detector (call it action because that is what it is currently doing)
    for(auto& detector : geo_manager_->getDetectors()) {
        auto model_g4 = detector->getExternalModel<DetectorModelG4>();

        // set maximum step length
        model_g4->pixel_log->SetUserLimits(user_limits_.get());

        // add the sensitive detector action
        auto sensitive_detector_action = new SensitiveDetectorActionG4(detector, messenger_, charge_creation_energy);
        model_g4->pixel_log->SetSensitiveDetector(sensitive_detector_action);
    }

    // disable verbose processes
    // FIXME: there should be a more general way to do it, but I have not found it yet
    G4UImanager* UI = G4UImanager::GetUIpointer();
    UI->ApplyCommand("/process/verbose 0");
    UI->ApplyCommand("/process/em/verbose 0");
    UI->ApplyCommand("/process/eLoss/verbose 0");
    G4HadronicProcessStore::Instance()->SetVerbose(0);

    // release output from G4
    RELEASE_STREAM(G4cout);
}

// run the deposition
void DepositionGeant4Module::run() {
    // start the beam
    LOG(INFO) << "Enabling beam";
    run_manager_g4_->BeamOn(1);
}

// External function, to allow loading from dynamic library without knowing module type.
// Should be overloaded in all module implementations, added here to prevent crashes
Module* allpix::generator(Configuration config, Messenger* messenger, GeometryManager* geometry) {
    DepositionGeant4Module* module = new DepositionGeant4Module(config, messenger, geometry);
    return dynamic_cast<Module*>(module);
}
