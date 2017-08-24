/**
 * @file
 * @brief Implementation of Geant4 deposition module
 * @remarks Based on code from Mathieu Benoit
 * @copyright MIT License
 */

#include "DepositionGeant4Module.hpp"

#include <limits>
#include <string>
#include <utility>

#include <G4EmParameters.hh>
#include <G4HadronicProcessStore.hh>
#include <G4LogicalVolume.hh>
#include <G4PhysListFactory.hh>
#include <G4RunManager.hh>
#include <G4StepLimiterPhysics.hh>
#include <G4UImanager.hh>
#include <G4UserLimits.hh>

#include "core/config/exceptions.h"
#include "core/geometry/GeometryManager.hpp"
#include "core/module/exceptions.h"
#include "core/utils/log.h"
#include "objects/DepositedCharge.hpp"
#include "tools/ROOT.h"
#include "tools/geant4.h"

#include "GeneratorActionG4.hpp"
#include "SensitiveDetectorActionG4.hpp"

#define G4_NUM_SEEDS 10

using namespace allpix;

/**
 * Includes the particle source point to the geometry using \ref GeometryManager::addPoint.
 */
DepositionGeant4Module::DepositionGeant4Module(Configuration config, Messenger* messenger, GeometryManager* geo_manager)
    : Module(config), config_(std::move(config)), messenger_(messenger), geo_manager_(geo_manager), last_event_num_(1),
      run_manager_g4_(nullptr) {
    // Create user limits for maximum step length in the sensor
    user_limits_ =
        std::make_unique<G4UserLimits>(config_.get<double>("max_step_length", std::numeric_limits<double>::max()));

    // Add the particle source position to the geometry
    geo_manager_->addPoint(config_.get<ROOT::Math::XYZPoint>("beam_position"));
}

/**
 * Module depends on \ref GeometryBuilderGeant4Module loaded first, because it owns the pointer to the Geant4 run manager.
 */
void DepositionGeant4Module::init() {
    // Load the G4 run manager (which is owned by the geometry builder)
    run_manager_g4_ = G4RunManager::GetRunManager();
    if(run_manager_g4_ == nullptr) {
        throw ModuleError("Cannot deposit charges using Geant4 without a Geant4 geometry builder");
    }

    // Suppress all output from G4
    SUPPRESS_STREAM(G4cout);

    // Get UI manager for sending commands
    G4UImanager* ui_g4 = G4UImanager::GetUIpointer();

    // Apply optional PAI model
    if(config_.get<bool>("enable_pai", false)) {
        LOG(TRACE) << "Enabling PAI model on all detectors";
        G4EmParameters::Instance();

        for(auto& detector : geo_manager_->getDetectors()) {
            // Get logical volume
            auto logical_volume = detector->getExternalObject<G4LogicalVolume>("sensor_log");
            if(logical_volume == nullptr) {
                throw ModuleError("Detector " + detector->getName() + " has no sensitive device (broken Geant4 geometry)");
            }
            // Create region
            G4Region* region = new G4Region(detector->getName() + "_sensor_region");
            region->AddRootLogicalVolume(logical_volume.get());

            auto pai_model = config_.get<std::string>("pai_model", "pai");
            auto lcase_model = pai_model;
            std::transform(lcase_model.begin(), lcase_model.end(), lcase_model.begin(), ::tolower);
            if(lcase_model == "pai") {
                pai_model = "PAI";
            } else if(lcase_model == "paiphoton") {
                pai_model = "PAIphoton";
            } else {
                throw InvalidValueError(config_, "pai_model", "model has to be either 'pai' or 'paiphoton'");
            }

            ui_g4->ApplyCommand("/process/em/AddPAIRegion all " + region->GetName() + " " + pai_model);
        }
    }

    // Find the physics list
    // FIXME Set a good default physics list
    G4PhysListFactory physListFactory;
    G4VModularPhysicsList* physicsList = physListFactory.GetReferencePhysList(config_.get<std::string>("physics_list"));
    if(physicsList == nullptr) {
        std::string message = "specified physics list does not exists";
        std::vector<G4String> base_lists = physListFactory.AvailablePhysLists();
        message += " (available base lists are ";
        for(auto& base_list : base_lists) {
            message += base_list;
            message += ", ";
        }
        message = message.substr(0, message.size() - 2);
        message += " with optional suffixes for electromagnetic lists ";
        std::vector<G4String> em_lists = physListFactory.AvailablePhysListsEM();
        for(auto& em_list : em_lists) {
            if(em_list.empty()) {
                continue;
            }
            message += em_list;
            message += ", ";
        }
        message = message.substr(0, message.size() - 2);
        message += ")";

        throw InvalidValueError(config_, "physics_list", message);
    } else {
        LOG(INFO) << "Using G4 physics list \"" << config_.get<std::string>("physics_list") << "\"";
    }
    // Register a step limiter (uses the user limits defined earlier)
    physicsList->RegisterPhysics(new G4StepLimiterPhysics());
    // Initialize the physics list
    LOG(TRACE) << "Initializing physics processes";
    run_manager_g4_->SetUserInitialization(physicsList);
    run_manager_g4_->InitializePhysics();

    // Initialize the full run manager to ensure correct state flags
    run_manager_g4_->Initialize();

    // Build particle generator
    LOG(TRACE) << "Constructing particle source";
    auto generator = new GeneratorActionG4(config_);
    run_manager_g4_->SetUserAction(generator);

    // Get the creation energy for charge (default is silicon electron hole pair energy)
    auto charge_creation_energy = config_.get<double>("charge_creation_energy", Units::get(3.64, "eV"));

    // Loop through all detectors and set the sensitive detector action that handles the particle passage
    bool useful_deposition = false;
    for(auto& detector : geo_manager_->getDetectors()) {
        // Do not add sensitive detector for detectors that have no listeners for the deposited charges
        // FIXME Probably the MCParticle has to be checked as well
        if(!messenger_->hasReceiver(this,
                                    std::make_shared<DepositedChargeMessage>(std::vector<DepositedCharge>(), detector))) {
            LOG(INFO) << "Not depositing charges in " << detector->getName()
                      << " because there is no listener for its output";
            continue;
        }
        useful_deposition = true;

        // Get model of the sensitive device
        auto sensitive_detector_action = new SensitiveDetectorActionG4(this, detector, messenger_, charge_creation_energy);
        auto logical_volume = detector->getExternalObject<G4LogicalVolume>("sensor_log");
        if(logical_volume == nullptr) {
            throw ModuleError("Detector " + detector->getName() + " has no sensitive device (broken Geant4 geometry)");
        }

        // Apply the user limits to this element
        logical_volume->SetUserLimits(user_limits_.get());

        // Add the sensitive detector action
        logical_volume->SetSensitiveDetector(sensitive_detector_action);
        sensors_.push_back(sensitive_detector_action);
    }

    if(!useful_deposition) {
        LOG(ERROR) << "Not a single listener for deposited charges, module is useless!";
    }

    // Disable verbose messages from processes
    ui_g4->ApplyCommand("/process/verbose 0");
    ui_g4->ApplyCommand("/process/em/verbose 0");
    ui_g4->ApplyCommand("/process/eLoss/verbose 0");
    G4HadronicProcessStore::Instance()->SetVerbose(0);

    // Set the random seed for Geant4 generation
    // NOTE Assumes this is the only Geant4 module using random numbers
    std::string seed_command = "/random/setSeeds ";
    for(int i = 0; i < G4_NUM_SEEDS; ++i) {
        seed_command += std::to_string(static_cast<uint32_t>(getRandomSeed() % INT_MAX));
        if(i != G4_NUM_SEEDS - 1) {
            seed_command += " ";
        }
    }
    ui_g4->ApplyCommand(seed_command);

    // Release the output stream
    RELEASE_STREAM(G4cout);
}

void DepositionGeant4Module::run(unsigned int event_num) {
    // Suppress output stream if not in debugging mode
    IFLOG(DEBUG);
    else {
        SUPPRESS_STREAM(G4cout);
    }

    // Start a single event from the beam
    LOG(TRACE) << "Enabling beam";
    run_manager_g4_->BeamOn(static_cast<int>(config_.get<unsigned int>("number_of_particles", 1)));
    last_event_num_ = event_num;

    // Release the stream (if it was suspended)
    RELEASE_STREAM(G4cout);

    // Dispatch the necessary messages
    for(auto& sensor : sensors_) {
        sensor->dispatchMessages();
    }
}

void DepositionGeant4Module::finalize() {
    size_t total_charges = 0;
    for(auto& sensor : sensors_) {
        total_charges += sensor->getTotalDepositedCharge();
    }

    // Print summary or warns if module did not output any charges
    if(!sensors_.empty() && total_charges > 0 && last_event_num_ > 0) {
        size_t average_charge = total_charges / sensors_.size() / last_event_num_;
        LOG(INFO) << "Deposited total of " << total_charges << " charges in " << sensors_.size() << " sensor(s) (average of "
                  << average_charge << " per sensor for every event)";
    } else {
        LOG(WARNING) << "No charges deposited";
    }
}
