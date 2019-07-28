/**
 * @file
 * @brief Implementation of Geant4 deposition module
 * @remarks Based on code from Mathieu Benoit
 * @copyright Copyright (c) 2017-2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "DepositionGeant4Module.hpp"

#include <limits>
#include <string>
#include <utility>

#include <G4EmParameters.hh>
#include <G4HadronicProcessStore.hh>
#include <G4LogicalVolume.hh>
#include <G4PhysListFactory.hh>
#include <G4RadioactiveDecayPhysics.hh>
#include <G4StepLimiterPhysics.hh>
#include <G4UImanager.hh>
#include <G4UserLimits.hh>

#include "G4FieldManager.hh"
#include "G4TransportationManager.hh"
#include "G4UniformMagField.hh"

#include "G4RunManager/MTRunManager.hpp"
#include "core/config/exceptions.h"
#include "core/geometry/GeometryManager.hpp"
#include "core/module/exceptions.h"
#include "core/utils/log.h"
#include "objects/DepositedCharge.hpp"
#include "tools/ROOT.h"
#include "tools/geant4.h"

#include "ActionInitializationG4.hpp"
#include "GeneratorActionG4.hpp"
#include "SDAndFieldConstruction.hpp"
#include "SensitiveDetectorActionG4.hpp"
#include "SetTrackInfoUserHookG4.hpp"

#define G4_NUM_SEEDS 10

using namespace allpix;

thread_local std::unique_ptr<TrackInfoManager> DepositionGeant4Module::track_info_manager_ = nullptr;
thread_local std::vector<SensitiveDetectorActionG4*> DepositionGeant4Module::sensors_;

/**
 * Includes the particle source point to the geometry using \ref GeometryManager::addPoint.
 */
DepositionGeant4Module::DepositionGeant4Module(Configuration& config, Messenger* messenger, GeometryManager* geo_manager)
    : Geant4Module(config), messenger_(messenger), geo_manager_(geo_manager), run_manager_g4_(nullptr) {
    // Create user limits for maximum step length in the sensor
    user_limits_ = std::make_unique<G4UserLimits>(config_.get<double>("max_step_length", Units::get(1.0, "um")));

    // Set default physics list
    config_.setDefault("physics_list", "FTFP_BERT_LIV");

    config_.setDefault("source_type", "beam");
    config_.setDefault<bool>("output_plots", false);
    config_.setDefault<int>("output_plots_scale", Units::get(100, "ke"));

    // Set alias for support of old particle source definition
    config_.setAlias("source_position", "beam_position");
    config_.setAlias("source_energy", "beam_energy");
    config_.setAlias("source_energy_spread", "beam_energy_spread");

    // If macro, parse for positions of sources and add these as points to the GeoManager to extend the world:
    if(config.get<std::string>("source_type") == "macro") {
        std::ifstream file(config.getPath("file_name", true));
        std::string line;
        while(std::getline(file, line)) {
            if(line.rfind("/gps/position", 0) == 0 || line.rfind("/gps/pos/centre") == 0) {
                LOG(TRACE) << "Macro contains source position: \"" << line << "\"";
                std::stringstream sstr(line);
                std::string command, units;
                double pos_x, pos_y, pos_z;
                sstr >> command >> pos_x >> pos_y >> pos_z >> units;
                ROOT::Math::XYZPoint source_position(
                    Units::get(pos_x, units), Units::get(pos_y, units), Units::get(pos_z, units));
                LOG(DEBUG) << "Found source positioned at " << Units::display(source_position, {"mm", "cm"});
                geo_manager_->addPoint(source_position);
            }
        }
    }

    // Add the particle source position to the geometry
    geo_manager_->addPoint(config_.get<ROOT::Math::XYZPoint>("source_position", ROOT::Math::XYZPoint()));
}

/**
 * Module depends on \ref GeometryBuilderGeant4Module loaded first, because it owns the pointer to the Geant4 run manager.
 */
void DepositionGeant4Module::init(std::mt19937_64& seeder) {
    Configuration& global_config = getConfigManager()->getGlobalConfiguration();
    MTRunManager* run_manager_mt = nullptr;

    // Load the G4 run manager (which is owned by the geometry builder)
    if(global_config.get<bool>("experimental_multithreading", false)) {
        run_manager_g4_ = G4MTRunManager::GetMasterRunManager();
        run_manager_mt = static_cast<MTRunManager*>(run_manager_g4_);
        G4Threading::SetMultithreadedApplication(true);
        using_multithreading_ = true;
    } else {
        run_manager_g4_ = G4RunManager::GetRunManager();
    }

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

    // Register radioactive decay physics lists
    physicsList->RegisterPhysics(new G4RadioactiveDecayPhysics());

    // Set the range-cut off threshold for secondary production:
    double production_cut;
    if(config_.has("range_cut")) {
        production_cut = config_.get<double>("range_cut");
        LOG(INFO) << "Setting configured G4 production cut to " << Units::display(production_cut, {"mm", "um"});
    } else {
        // Define the production cut as one fifth of the minimum size (thickness, pitch) among the detectors
        double min_size = std::numeric_limits<double>::max();
        std::string min_detector;
        for(auto& detector : geo_manager_->getDetectors()) {
            auto model = detector->getModel();
            double prev_min_size = min_size;
            min_size =
                std::min({min_size, model->getPixelSize().x(), model->getPixelSize().y(), model->getSensorSize().z()});
            if(min_size != prev_min_size) {
                min_detector = detector->getName();
            }
        }
        production_cut = min_size / 5;
        LOG(INFO) << "Setting G4 production cut to " << Units::display(production_cut, {"mm", "um"})
                  << ", derived from properties of detector \"" << min_detector << "\"";
    }
    ui_g4->ApplyCommand("/run/setCut " + std::to_string(production_cut));

    // Initialize the physics list
    LOG(TRACE) << "Initializing physics processes";
    run_manager_g4_->SetUserInitialization(physicsList);
    run_manager_g4_->InitializePhysics();

    // Prepare seeds for Geant4:
    // NOTE Assumes this is the only Geant4 module using random numbers
    std::string seed_command = "/random/setSeeds ";
    for(int i = 0; i < G4_NUM_SEEDS; ++i) {
        seed_command += std::to_string(static_cast<uint32_t>(seeder() % INT_MAX));
        if(i != G4_NUM_SEEDS - 1) {
            seed_command += " ";
        }
    }

    // Set the random seed for Geant4 generation before calling initialize
    // since it draws random numbers
    ui_g4->ApplyCommand(seed_command);

    // Initialize the full run manager to ensure correct state flags
    run_manager_g4_->Initialize();

    // Build particle generator
    // User hook to store additional information at track initialization and termination as well as custom track ids
    LOG(TRACE) << "Constructing particle source";

    auto action_initialization = new ActionInitializationG4(config_, this);
    run_manager_g4_->SetUserInitialization(action_initialization);

    // Get the creation energy for charge (default is silicon electron hole pair energy)
    auto charge_creation_energy = config_.get<double>("charge_creation_energy", Units::get(3.64, "eV"));
    auto fano_factor = config_.get<double>("fano_factor", 0.115);

    // Construct the sensitive detectors and fields. In MT-mode we register a builder that will be called for each
    // thread to construct the SD when needed. While in sequential mode we construct it now.
    if(using_multithreading_) {
        auto detector_construction = new SDAndFieldConstruction(this, fano_factor, charge_creation_energy);
        run_manager_mt->SetSDAndFieldConstruction(detector_construction);
    } else {
        // Create the info track manager for the main thread before creating the Sensitive detectors.
        track_info_manager_ = std::make_unique<TrackInfoManager>();
        construct_sensitive_detectors_and_fields(fano_factor, charge_creation_energy);
    }

    // Disable verbose messages from processes
    ui_g4->ApplyCommand("/process/verbose 0");
    ui_g4->ApplyCommand("/process/em/verbose 0");
    ui_g4->ApplyCommand("/process/eLoss/verbose 0");
    G4HadronicProcessStore::Instance()->SetVerbose(0);

    // Release the output stream
    RELEASE_STREAM(G4cout);
}

void DepositionGeant4Module::run(Event* event) {
    MTRunManager* run_manager_mt = nullptr;

    // Initialize the thread local G4RunManager in case of MT
    if(using_multithreading_) {
        run_manager_mt = static_cast<MTRunManager*>(run_manager_g4_);

        // In MT-mode the sensitive detectors will be created with the calls to BeamOn. So we construct the
        // track manager for each calling thread here.
        if(track_info_manager_ == nullptr) {
            track_info_manager_ = std::make_unique<TrackInfoManager>();
        }

        run_manager_mt->InitializeForThread();
    }

    // Suppress output stream if not in debugging mode
    IFLOG(DEBUG);
    else {
        std::lock_guard<std::mutex> lock(g4cout_mutex_);
        SUPPRESS_STREAM(G4cout);
    }

    // Seed the sensitive detectors RNG
    for(auto& sensor : sensors_) {
        sensor->seed(event->getRandomNumber());
    }

    // Start a single event from the beam
    LOG(TRACE) << "Enabling beam";
    if(using_multithreading_) {
        run_manager_mt->Run(static_cast<G4int>(event->number),
                            static_cast<int>(config_.get<unsigned int>("number_of_particles", 1)));
    } else {
        run_manager_g4_->BeamOn(static_cast<int>(config_.get<unsigned int>("number_of_particles", 1)));
    }

    unsigned int last_event_num = last_event_num_.load();
    last_event_num_.compare_exchange_strong(last_event_num, event->number);

    {
        // Release the stream (if it was suspended)
        std::lock_guard<std::mutex> lock(g4cout_mutex_);
        RELEASE_STREAM(G4cout);
    }

    track_info_manager_->createMCTracks();

    // Dispatch the necessary messages
    for(auto& sensor : sensors_) {
        sensor->dispatchMessages(event);

        // Fill output plots if requested:
        if(config_.get<bool>("output_plots")) {
            double charge = static_cast<double>(Units::convert(sensor->getDepositedCharge(), "ke"));
            auto histogram = charge_per_event_[sensor->getName()]->Get();
            histogram->Fill(charge);
        }
    }

    track_info_manager_->dispatchMessage(event);
    track_info_manager_->resetTrackInfoManager();
}

void DepositionGeant4Module::finalize() {
    if(config_.get<bool>("output_plots")) {
        // Write histograms
        LOG(TRACE) << "Writing output plots to file";
        for(auto& histogram : charge_per_event_) {
            auto plot = histogram.second->Merge();
            plot->Write();
        }
    }

    // Record the number of sensors and the total charges
    if(!using_multithreading_) {
        record_module_statistics();
    }

    // Print summary or warns if module did not output any charges
    if(number_of_sensors_ > 0 && total_charges_ > 0 && last_event_num_ > 0) {
        size_t average_charge = total_charges_ / number_of_sensors_ / last_event_num_;
        LOG(INFO) << "Deposited total of " << total_charges_ << " charges in " << number_of_sensors_
                  << " sensor(s) (average of " << average_charge << " per sensor for every event)";
    } else {
        LOG(WARNING) << "No charges deposited";
    }
}

void DepositionGeant4Module::finalizeThread() {
    auto run_manager_mt = static_cast<MTRunManager*>(run_manager_g4_);
    run_manager_mt->TerminateForThread();

    // Record the number of sensors and the total charges
    record_module_statistics();
}

void DepositionGeant4Module::construct_sensitive_detectors_and_fields(double fano_factor, double charge_creation_energy) {
    if(geo_manager_->hasMagneticField()) {
        MagneticFieldType magnetic_field_type_ = geo_manager_->getMagneticFieldType();

        if(magnetic_field_type_ == MagneticFieldType::CONSTANT) {
            ROOT::Math::XYZVector b_field = geo_manager_->getMagneticField(ROOT::Math::XYZPoint(0., 0., 0.));
            G4MagneticField* magField = new G4UniformMagField(G4ThreeVector(b_field.x(), b_field.y(), b_field.z()));
            G4FieldManager* globalFieldMgr = G4TransportationManager::GetTransportationManager()->GetFieldManager();
            globalFieldMgr->SetDetectorField(magField);
            globalFieldMgr->CreateChordFinder(magField);
        } else {
            throw ModuleError("Magnetic field enabled, but not constant. This can't be handled by this module yet.");
        }
    }

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
        auto sensitive_detector_action =
            new SensitiveDetectorActionG4(detector, track_info_manager_.get(), charge_creation_energy, fano_factor);
        auto logical_volume = detector->getExternalObject<G4LogicalVolume>("sensor_log");
        if(logical_volume == nullptr) {
            throw ModuleError("Detector " + detector->getName() + " has no sensitive device (broken Geant4 geometry)");
        }

        // Apply the user limits to this element
        logical_volume->SetUserLimits(user_limits_.get());

        // Add the sensitive detector action
        logical_volume->SetSensitiveDetector(sensitive_detector_action);
        sensors_.push_back(sensitive_detector_action);

        // If requested, prepare output plots
        if(config_.get<bool>("output_plots")) {
            LOG(TRACE) << "Creating output plots";

            // Plot axis are in kilo electrons - convert from framework units!
            int maximum = static_cast<int>(Units::convert(config_.get<int>("output_plots_scale"), "ke"));
            int nbins = 5 * maximum;

            // Create histograms if needed
            std::string plot_name = "deposited_charge_" + sensitive_detector_action->getName();

            {
                std::lock_guard<std::mutex> lock(histogram_mutex_);
                if(charge_per_event_.find(sensitive_detector_action->getName()) == charge_per_event_.end()) {
                    charge_per_event_[sensitive_detector_action->getName()] = new ROOT::TThreadedObject<TH1D>(
                        plot_name.c_str(), "deposited charge per event;deposited charge [ke];events", nbins, 0, maximum);
                }
            }
        }
    }

    if(!useful_deposition) {
        LOG(ERROR) << "Not a single listener for deposited charges, module is useless!";
    }
}

void DepositionGeant4Module::record_module_statistics() {
    number_of_sensors_ = sensors_.size();

    // We calculate the total deposited charges here, since sensors exist per thread
    for(auto& sensor : sensors_) {
        total_charges_ += sensor->getTotalDepositedCharge();
    }
}
