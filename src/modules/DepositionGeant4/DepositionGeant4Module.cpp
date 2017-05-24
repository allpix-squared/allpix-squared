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

#include "core/config/exceptions.h"
#include "core/geometry/GeometryManager.hpp"
#include "core/module/exceptions.h"
#include "core/utils/log.h"
#include "core/utils/random.h"
#include "tools/geant4.h"

#include "objects/DepositedCharge.hpp"

#include "GeneratorActionG4.hpp"
#include "SensitiveDetectorActionG4.hpp"

// FIXME: broken common includes
#include "modules/common/DetectorModelG4.hpp"

using namespace allpix;

DepositionGeant4Module::DepositionGeant4Module(Configuration config, Messenger* messenger, GeometryManager* geo_manager)
    : config_(std::move(config)), messenger_(messenger), geo_manager_(geo_manager), last_event_num_(1),
      run_manager_g4_(nullptr) {
    // create user limits for maximum step length in the sensor
    user_limits_ =
        std::make_unique<G4UserLimits>(config_.get<double>("max_step_length", std::numeric_limits<double>::max()));
}

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
        RELEASE_STREAM(G4cout);
        // FIXME: more information about available lists
        throw InvalidValueError(config_, "physics_list", "specified physics list does not exists");
    }
    // register a step limiter
    physicsList->RegisterPhysics(new G4StepLimiterPhysics());
    // initialize the physics list
    LOG(TRACE) << "Initializing physics processes";
    run_manager_g4_->SetUserInitialization(physicsList);
    run_manager_g4_->InitializePhysics();

    // initialize the full run manager to ensure correct state flags
    run_manager_g4_->Initialize();

    // build generator
    LOG(TRACE) << "Constructing particle source";
    auto generator = new GeneratorActionG4(config_);
    run_manager_g4_->SetUserAction(generator);

    // get the creation energy for charge (default is silicon electron hole pair energy)
    // FIXME: is this a good name...
    auto charge_creation_energy = config_.get<double>("charge_creation_energy", 3.64e-6);

    // loop through all detectors and set the sensitive detector (call it action because that is what it is currently doing)
    bool useful_deposition = false;
    for(auto& detector : geo_manager_->getDetectors()) {
        // do not add sensitive detector for detectors that have no listeners
        if(!messenger_->hasReceiver(std::make_shared<DepositedChargeMessage>(std::vector<DepositedCharge>(), detector),
                                    "sensor")) {
            LOG(INFO) << "Not depositing charges in " << detector->getName()
                      << " because there is no listener for its output";
            continue;
        }
        useful_deposition = true;

        // get model of the detector
        auto model_g4 = detector->getExternalModel<DetectorModelG4>();

        // set maximum step length
        model_g4->pixel_log->SetUserLimits(user_limits_.get());

        // add the sensitive detector action
        auto sensitive_detector_action = new SensitiveDetectorActionG4(this, detector, messenger_, charge_creation_energy);
        model_g4->pixel_log->SetSensitiveDetector(sensitive_detector_action);
        sensors_.push_back(sensitive_detector_action);
    }

    if(!useful_deposition) {
        LOG(ERROR) << "Not a single listener for deposited charges, module is useless!";
    }

    // disable verbose processes
    // FIXME: there should be a more general way to do it, but I have not found it yet
    G4UImanager* UI = G4UImanager::GetUIpointer();
    UI->ApplyCommand("/process/verbose 0");
    UI->ApplyCommand("/process/em/verbose 0");
    UI->ApplyCommand("/process/eLoss/verbose 0");
    G4HadronicProcessStore::Instance()->SetVerbose(0);

    // set the seed
    std::string seed_command = "/random/setSeeds ";
    seed_command += std::to_string(static_cast<uint32_t>(get_random_seed() % UINT_MAX));
    seed_command += " ";
    seed_command += std::to_string(static_cast<uint32_t>(get_random_seed() % UINT_MAX));
    UI->ApplyCommand(seed_command);

    // release output from G4
    RELEASE_STREAM(G4cout);
}

// run the deposition
void DepositionGeant4Module::run(unsigned int event_num) {
    // suppress stream if not in debugging mode
    IFLOG(DEBUG);
    else {
        SUPPRESS_STREAM(G4cout);
    }

    // start the beam
    LOG(TRACE) << "Enabling beam";
    run_manager_g4_->BeamOn(1);
    last_event_num_ = event_num;

    // release the stream (if it was suspended)
    RELEASE_STREAM(G4cout);
}

// print summary
void DepositionGeant4Module::finalize() {
    size_t total_charges = 0;
    for(auto& sensor : sensors_) {
        total_charges += sensor->getTotalDepositedCharge();
    }

    if(sensors_.size() > 0 && total_charges > 0) {
        size_t average_charge = total_charges / sensors_.size() / last_event_num_;
        LOG(INFO) << "Deposited total of " << total_charges << " charges in " << sensors_.size() << " sensor(s) (average of "
                  << average_charge << " per sensor for every event)";
    } else {
        LOG(WARNING) << "No charges deposited";
    }
}
