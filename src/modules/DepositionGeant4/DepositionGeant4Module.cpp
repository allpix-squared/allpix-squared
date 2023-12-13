/**
 * @file
 * @brief Implementation of Geant4 deposition module
 * @remarks Based on code from Mathieu Benoit
 *
 * @copyright Copyright (c) 2017-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "DepositionGeant4Module.hpp"

#include <limits>
#include <string>
#include <utility>

#include <G4Box.hh>
#include <G4EmParameters.hh>
#include <G4HadronicParameters.hh>
#include <G4HadronicProcessStore.hh>
#include <G4LogicalVolume.hh>
#include <G4NuclearLevelData.hh>
#include <G4PhysListFactory.hh>
#include <G4ProcessTable.hh>
#include <G4RadioactiveDecayPhysics.hh>
#include <G4StepLimiterPhysics.hh>
#include <G4UImanager.hh>
#include <G4UserLimits.hh>

#include "G4FieldManager.hh"
#include "G4TransportationManager.hh"
#include "G4UniformMagField.hh"

#include "core/config/exceptions.h"
#include "core/geometry/GeometryManager.hpp"
#include "core/geometry/RadialStripDetectorModel.hpp"
#include "core/module/exceptions.h"
#include "core/utils/log.h"
#include "objects/DepositedCharge.hpp"
#include "physics/MaterialProperties.hpp"
#include "tools/ROOT.h"
#include "tools/geant4/MTRunManager.hpp"
#include "tools/geant4/RunManager.hpp"
#include "tools/geant4/geant4.h"

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
    : SequentialModule(config), messenger_(messenger), geo_manager_(geo_manager) {
    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();
    // Waive any sequence requirement: base module not sequential, but derived modules might be
    waive_sequence_requirement();

    // Set default physics list
    config_.setDefault("physics_list", "FTFP_BERT_LIV");
    config_.setDefault("pai_model", "pai");

    config_.setDefault("source_type", "beam");
    config_.setDefault<bool>("output_plots", false);
    config_.setDefault<int>("output_plots_scale", Units::get(100, "ke"));
    config_.setDefault<double>("max_step_length", Units::get(1.0, "um"));
    // Default value chosen to ensure proper gamma generation for Cs137 decay
    config_.setDefault<double>("cutoff_time", 2.21e+11);
    // By default, only record MCTracks connected to MCParticles in the sensitive volume
    config_.setDefault<bool>("record_all_tracks", false);

    // Defaults for energy deposition in implants
    config_.setDefault<bool>("deposit_in_frontside_implants", true);
    config_.setDefault<bool>("deposit_in_backside_implants", false);

    // Create user limits for maximum step length and maximum event time in the sensor
    user_limits_ =
        std::make_unique<G4UserLimits>(config_.get<double>("max_step_length"), DBL_MAX, config_.get<double>("cutoff_time"));

    // Create user limits for maximum event time in the world volume:
    user_limits_world_ = std::make_unique<G4UserLimits>(DBL_MAX, DBL_MAX, config_.get<double>("cutoff_time"));

    // If macro, parse for positions of sources and add these as points to the GeoManager to extend the world:
    if(config.get<GeneratorActionG4::SourceType>("source_type") == GeneratorActionG4::SourceType::MACRO) {
        std::ifstream file(config.getPath("file_name", true));
        std::string line;
        while(std::getline(file, line)) {
            if(line.rfind("/gps/position", 0) == 0 || line.rfind("/gps/pos/centre") == 0) {
                LOG(TRACE) << "Macro contains source position: \"" << line << "\"";
                std::stringstream sstr(line);
                std::string command, units;
                double pos_x = NAN, pos_y = NAN, pos_z = NAN;
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
void DepositionGeant4Module::initialize() {
    MTRunManager* run_manager_mt = nullptr;

    number_of_particles_ = config_.get<unsigned int>("number_of_particles", 1);
    output_plots_ = config_.get<bool>("output_plots");

    // Load the G4 run manager (which is owned by the geometry builder)
    if(multithreadingEnabled()) {
        run_manager_g4_ = G4MTRunManager::GetMasterRunManager();
        run_manager_mt = static_cast<MTRunManager*>(run_manager_g4_);
        G4Threading::SetMultithreadedApplication(true);
    } else {
        run_manager_g4_ = G4RunManager::GetRunManager();
    }

    if(run_manager_g4_ == nullptr) {
        throw ModuleError("Cannot deposit charges using Geant4 without a Geant4 geometry builder");
    }

    // Apply optional PAI model
    if(config_.get<bool>("enable_pai", false)) {
        LOG(TRACE) << "Enabling PAI model on all detectors";
        G4EmParameters::Instance();

        auto pai_model = config_.get<std::string>("pai_model");
        auto lcase_model = allpix::transform(pai_model, ::tolower);
        if(lcase_model == "pai") {
            pai_model = "PAI";
        } else if(lcase_model == "paiphoton") {
            pai_model = "PAIphoton";
        } else {
            throw InvalidValueError(config_, "pai_model", "model has to be either 'pai' or 'paiphoton'");
        }

        for(auto& detector : geo_manager_->getDetectors()) {
            // Get logical volume
            auto logical_volume = geo_manager_->getExternalObject<G4LogicalVolume>(detector->getName(), "sensor_log");
            if(logical_volume == nullptr) {
                throw ModuleError("Detector " + detector->getName() + " has no sensitive device (broken Geant4 geometry)");
            }
            // Create region
            auto* region = new G4Region(detector->getName() + "_sensor_region");
            region->AddRootLogicalVolume(logical_volume.get());

            G4EmParameters::Instance()->AddPAIModel("all", region->GetName(), pai_model);
        }
    }

    // Find the physics list
    auto physics_list = config_.get<std::string>("physics_list");
    G4PhysListFactory physListFactory;
    G4VModularPhysicsList* physicsList = physListFactory.GetReferencePhysList(physics_list);
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
        LOG(INFO) << "Using G4 physics list \"" << physics_list << "\"";
    }
    // Register a step limiter (uses the user limits defined earlier)
    LOG(DEBUG) << "Registering Geant4 step limiter physics list";
    physicsList->RegisterPhysics(new G4StepLimiterPhysics());

    // Register radioactive decay physics lists unless we are using a _HP list which include this already:
    if(physics_list.find("_HP") == std::string::npos) {
        LOG(DEBUG) << "Registering Geant4 radioactive decay physics list";
        physicsList->RegisterPhysics(new G4RadioactiveDecayPhysics());
    }

    // Set the range-cut off threshold for secondary production:
    double production_cut = NAN;
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
    physicsList->SetDefaultCutValue(production_cut);

    // Set minimum remaining kinetic energy for a track
    double min_charge_creation_energy{};
    if(config_.has("charge_creation_energy")) {
        min_charge_creation_energy = config_.get<double>("charge_creation_energy");
        LOG(INFO) << "Setting minimum kinetic energy for tracks to " << Units::display(min_charge_creation_energy, {"eV"});
    } else {
        // Find minimum ionization energy from used detectors
        min_charge_creation_energy = std::numeric_limits<double>::max();
        std::string min_detector;
        for(auto& detector : geo_manager_->getDetectors()) {
            double this_min_charge_creation_energy = allpix::ionization_energies[detector->getModel()->getSensorMaterial()];
            if(this_min_charge_creation_energy < min_charge_creation_energy) {
                min_charge_creation_energy = this_min_charge_creation_energy;
                min_detector = detector->getName();
            }
        }
        LOG(INFO) << "Setting minimum kinetic energy for tracks to " << Units::display(min_charge_creation_energy, {"eV"})
                  << ", derived from material of detector \"" << min_detector << "\"";
    }
    user_limits_world_->SetUserMinEkine(min_charge_creation_energy);

    // Set user limits on world volume:
    auto world_log_volume = geo_manager_->getExternalObject<G4LogicalVolume>("", "world_log");
    if(world_log_volume != nullptr) {
        // Quickly estimate longest distance in world and limit max track length
        auto* world_box = static_cast<G4Box*>(world_log_volume->GetSolid());
        auto max_track_length =
            2e2 * (world_box->GetXHalfLength() + world_box->GetYHalfLength() + world_box->GetZHalfLength());
        user_limits_world_->SetUserMaxTrackLength(max_track_length);

        LOG(DEBUG) << "Setting world volume user limits to constrain event time to "
                   << Units::display(config_.get<double>("cutoff_time"), {"ns", "us", "ms", "s"})
                   << " and maximum track length to " << Units::display(max_track_length, {"mm", "cm", "m"});
        world_log_volume->GetRegion()->SetUserLimits(user_limits_world_.get());
    }

    // Initialize the physics list
    LOG(TRACE) << "Initializing physics processes";
    run_manager_g4_->SetUserInitialization(physicsList);
    run_manager_g4_->InitializePhysics();

    // Disable verbose messages from processes
    physicsList->SetVerboseLevel(0);
    G4ProcessTable::GetProcessTable()->SetVerboseLevel(0);
    G4EmParameters::Instance()->SetVerbose(0);
    G4HadronicProcessStore::Instance()->SetVerbose(0);
    G4HadronicParameters::Instance()->SetVerboseLevel(0);
    G4NuclearLevelData::GetInstance()->GetParameters()->SetVerbose(0);

    // Initialize the full run manager to ensure correct state flags
    run_manager_g4_->Initialize();

    // Build particle generator
    // User hook to store additional information at track initialization and termination as well as custom track ids
    LOG(TRACE) << "Constructing particle source";
    initialize_g4_action();

    // Construct the sensitive detectors and fields.
    if(run_manager_mt == nullptr) {
        // Create the info track manager for the main thread before creating the Sensitive detectors.
        track_info_manager_ = std::make_unique<TrackInfoManager>(config_.get<bool>("record_all_tracks"));
        construct_sensitive_detectors_and_fields();
    } else {
        // In MT-mode we register a builder that will be called for each thread to construct the SD when needed.
        auto detector_construction = std::make_unique<SDAndFieldConstruction>(this);
        run_manager_mt->SetSDAndFieldConstruction(std::move(detector_construction));
    }

    // Flush the Geant4 stream buffer because some elements in the initialization never do:
    G4cout << G4endl;
}

void DepositionGeant4Module::initialize_g4_action() {
    auto* action_initialization =
        new ActionInitializationG4<GeneratorActionG4, GeneratorActionInitializationMaster>(config_);
    run_manager_g4_->SetUserInitialization(action_initialization);
}

void DepositionGeant4Module::initializeThread() {

    LOG(DEBUG) << "Initializing run manager";

    // Initialize the thread local G4RunManager in case of MT
    if(multithreadingEnabled()) {
        auto* run_manager_mt = static_cast<MTRunManager*>(run_manager_g4_);

        // In MT-mode the sensitive detectors will be created with the calls to BeamOn. So we construct the
        // track manager for each calling thread here.
        if(track_info_manager_ == nullptr) {
            track_info_manager_ = std::make_unique<TrackInfoManager>(config_.get<bool>("record_all_tracks"));
        }

        run_manager_mt->InitializeForThread();
    }

    // Set selected tracking verbosity, defaulting to zero. Higher levels can be useful for tracing individual Geant4 events
    G4RunManagerKernel::GetRunManagerKernel()->GetTrackingManager()->SetVerboseLevel(
        config_.get<int>("geant4_tracking_verbosity", 0));
}

void DepositionGeant4Module::run(Event* event) {

    // Seed the sensitive detectors RNG
    for(auto& sensor : sensors_) {
        sensor->seed(event->getRandomNumber());
    }

    // Start a single event from the beam
    LOG(TRACE) << "Enabling beam";
    auto seed1 = event->getRandomNumber();
    auto seed2 = event->getRandomNumber();
    LOG(DEBUG) << "Seeding Geant4 event with seeds " << seed1 << " " << seed2;

    try {
        if(multithreadingEnabled()) {
            auto* run_manager_mt = static_cast<MTRunManager*>(run_manager_g4_);
            run_manager_mt->Run(static_cast<int>(number_of_particles_), seed1, seed2);
        } else {
            auto* run_manager = static_cast<RunManager*>(run_manager_g4_);
            run_manager->Run(static_cast<int>(number_of_particles_), seed1, seed2);
        }

        uint64_t last_event_num = last_event_num_.load();
        last_event_num_.compare_exchange_strong(last_event_num, event->number);

        track_info_manager_->createMCTracks();
        track_info_manager_->dispatchMessage(this, messenger_, event);

        // Dispatch the necessary messages
        for(auto& sensor : sensors_) {
            sensor->dispatchMessages(this, messenger_, event);

            // Fill output plots if requested:
            if(output_plots_) {
                double charge = static_cast<double>(Units::convert(sensor->getDepositedCharge(), "ke"));
                charge_per_event_[sensor->getName()]->Fill(charge);

                double deposited_energy = static_cast<double>(Units::convert(sensor->getDepositedEnergy(), "keV"));
                energy_per_event_[sensor->getName()]->Fill(deposited_energy);
            }
        }
    } catch(AbortEventException& e) {
        // Clear charge deposits of all sensors
        for(auto& sensor : sensors_) {
            sensor->clearEventInfo();
        }
        run_manager_g4_->AbortRun();
        track_info_manager_->resetTrackInfoManager();
        throw;
    }

    track_info_manager_->resetTrackInfoManager();
}

void DepositionGeant4Module::finalize() {
    if(output_plots_) {
        // Write histograms
        LOG(TRACE) << "Writing output plots to file";
        for(auto& histogram : charge_per_event_) {
            histogram.second->Write();
        }
        for(auto& histogram : energy_per_event_) {
            histogram.second->Write();
        }
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
    // Record the number of sensors and the total charges
    record_module_statistics();

    if(multithreadingEnabled()) {
        auto* run_manager_mt = static_cast<MTRunManager*>(run_manager_g4_);
        run_manager_mt->TerminateForThread();
    }
}

void DepositionGeant4Module::construct_sensitive_detectors_and_fields() {
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
        if(!messenger_->hasReceiver(this,
                                    std::make_shared<DepositedChargeMessage>(std::vector<DepositedCharge>(), detector)) &&
           !messenger_->hasReceiver(this, std::make_shared<MCParticleMessage>(std::vector<MCParticle>(), detector)) &&
           !messenger_->hasReceiver(this, std::make_shared<MCTrackMessage>(std::vector<MCTrack>()))) {
            LOG(INFO) << "Not depositing charges in " << detector->getName()
                      << " because there is no listener for its output";
            continue;
        }
        useful_deposition = true;

        // Get ionization energy and Fano factor
        auto model = detector->getModel();
        auto charge_creation_energy =
            (config_.has("charge_creation_energy") ? config_.get<double>("charge_creation_energy")
                                                   : allpix::ionization_energies[model->getSensorMaterial()]);
        auto fano_factor = (config_.has("fano_factor") ? config_.get<double>("fano_factor")
                                                       : allpix::fano_factors[model->getSensorMaterial()]);
        LOG(DEBUG) << "Detector " << detector->getName() << " uses charge creation energy "
                   << Units::display(charge_creation_energy, "eV") << " and Fano factor " << fano_factor;

        // Cut-off time for particle generation:
        auto cutoff_time = config_.get<double>("cutoff_time");

        // Get model of the sensitive device
        auto* sensitive_detector_action = new SensitiveDetectorActionG4(
            detector, track_info_manager_.get(), charge_creation_energy, fano_factor, cutoff_time);
        auto logical_volume = geo_manager_->getExternalObject<G4LogicalVolume>(detector->getName(), "sensor_log");
        if(logical_volume == nullptr) {
            throw ModuleError("Detector " + detector->getName() + " has no sensitive device (broken Geant4 geometry)");
        }

        // Apply the user limits to this element
        logical_volume->SetUserLimits(user_limits_.get());

        // Add the sensitive detector action
        logical_volume->SetSensitiveDetector(sensitive_detector_action);

        // Add the sensitive detector action to fronmtside implant volumes
        std::regex regex;
        if(config_.get<bool>("deposit_in_frontside_implants") && config_.get<bool>("deposit_in_backside_implants")) {
            regex = "implant_log_.*";
        } else if(config_.get<bool>("deposit_in_frontside_implants")) {
            regex = "implant_log_frontside_.*";
        } else if(config_.get<bool>("deposit_in_backside_implants")) {
            regex = "implant_log_backside_.*";
        }
        for(const auto& implant : geo_manager_->getExternalObjects<G4LogicalVolume>(detector->getName(), regex)) {
            implant->SetUserLimits(user_limits_.get());
            implant->SetSensitiveDetector(sensitive_detector_action);
        }

        sensors_.push_back(sensitive_detector_action);

        // If requested, prepare output plots
        if(output_plots_) {
            LOG(TRACE) << "Creating output plots for detector " << sensitive_detector_action->getName();

            // Plot axis are in kilo electrons - convert from framework units!
            int maximum_charge = static_cast<int>(Units::convert(config_.get<int>("output_plots_scale"), "ke"));
            double maximum_energy =
                (static_cast<int>(maximum_charge / 2. * Units::convert(charge_creation_energy, "eV")) / 10) * 10 + 10;
            int nbins = 5 * maximum_charge;

            // Create histograms if needed
            {
                std::lock_guard<std::mutex> lock(histogram_mutex_);
                std::string plot_name = "deposited_charge_" + sensitive_detector_action->getName();

                if(charge_per_event_.find(sensitive_detector_action->getName()) == charge_per_event_.end()) {
                    charge_per_event_[sensitive_detector_action->getName()] =
                        CreateHistogram<TH1D>(plot_name.c_str(),
                                              "deposited charge per event;deposited charge [ke];events",
                                              nbins,
                                              0,
                                              maximum_charge);
                }

                plot_name = "deposited_energy_" + sensitive_detector_action->getName();

                if(energy_per_event_.find(sensitive_detector_action->getName()) == energy_per_event_.end()) {
                    energy_per_event_[sensitive_detector_action->getName()] =
                        CreateHistogram<TH1D>(plot_name.c_str(),
                                              "deposited energy per event;deposited energy [keV];events",
                                              nbins,
                                              0,
                                              maximum_energy);
                }
            }
        }
    }

    if(!useful_deposition) {
        LOG(ERROR) << "Not a single listener for deposited charges, module is useless!";
    }
}

void DepositionGeant4Module::record_module_statistics() {
    // Since sensors is thread local, some instances may not be used, hence, skip them
    auto num_sensors = sensors_.size();
    if(num_sensors > 0) {
        number_of_sensors_ = num_sensors;
    }

    // We calculate the total deposited charges here, since sensors exist per thread
    for(auto& sensor : sensors_) {
        total_charges_ += sensor->getTotalDepositedCharge();
    }
}
