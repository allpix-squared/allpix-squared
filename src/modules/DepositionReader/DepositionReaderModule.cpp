/**
 * @file
 * @brief Implementation of DepositionReader module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "DepositionReaderModule.hpp"

#include <string>
#include <utility>

#include "core/utils/log.h"

using namespace allpix;

DepositionReaderModule::DepositionReaderModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager)
    : Module(config), geo_manager_(geo_manager), messenger_(messenger) {

    // Seed the random generator for Fano fluctuations with the seed received
    random_generator_.seed(getRandomSeed());
}

void DepositionReaderModule::init() {

    // Check which file type we want to read:
    file_model_ = config_.get<std::string>("model");
    std::transform(file_model_.begin(), file_model_.end(), file_model_.begin(), ::tolower);
    auto file_path = config_.getPath("file_name", true);
    if(file_model_ == "csv") {
        // Open the file with the objects
        input_file_ = std::make_unique<std::ifstream>(file_path);
        if(!input_file_->is_open()) {
            throw InvalidValueError(config_, "file_name", "could not open input file");
        }
    } else if(file_model_ == "root") {
        input_file_root_ = std::make_unique<TFile>(file_path.c_str(), "READ");
        if(!input_file_root_->IsOpen()) {
            throw InvalidValueError(config_, "file_name", "could not open input file");
        }
        input_file_root_->cd();
        tree_reader_ = std::make_shared<TTreeReader>("hitTree", input_file_root_.get());
        LOG(DEBUG) << "Initialized tree reader, found " << tree_reader_->GetEntries(false) << " entries";

        event_ = std::make_shared<TTreeReaderValue<int>>(*tree_reader_, "event");
        edep_ = std::make_shared<TTreeReaderValue<double>>(*tree_reader_, "energy.Edep");
        time_ = std::make_shared<TTreeReaderValue<double>>(*tree_reader_, "time");
        px_ = std::make_shared<TTreeReaderValue<double>>(*tree_reader_, "position.x");
        py_ = std::make_shared<TTreeReaderValue<double>>(*tree_reader_, "position.y");
        pz_ = std::make_shared<TTreeReaderValue<double>>(*tree_reader_, "position.z");
        volume_ = std::make_shared<TTreeReaderArray<char>>(*tree_reader_, "detector");
        pdg_code_ = std::make_shared<TTreeReaderValue<int>>(*tree_reader_, "PDG_code");

        tree_reader_->Next();
    } else {
        throw InvalidValueError(config_, "model", "only model 'csv' is currently supported");
    }

    // Get the creation energy for charge (default is silicon electron hole pair energy)
    charge_creation_energy_ = config_.get<double>("charge_creation_energy", Units::get(3.64, "eV"));
    fano_factor_ = config_.get<double>("fano_factor", 0.115);
}

void DepositionReaderModule::run(unsigned int event) {

    // Set of deposited charges in this event
    DepositMap deposits;
    ParticleMap mc_particles;
    ParticleRelationMap particles_to_deposits;

    LOG(DEBUG) << "Start reading event " << event;

    if(file_model_ == "csv")
        read_csv(event, deposits, mc_particles, particles_to_deposits);
    else if(file_model_ == "root")
        read_root(event, deposits, mc_particles, particles_to_deposits);

    LOG(DEBUG) << "Finished reading event " << event;

    // Loop over all known detectors and dispatch messages for them
    for(const auto& detector : geo_manager_->getDetectors()) {
        LOG(DEBUG) << "Detector " << detector->getName() << " has " << mc_particles[detector].size() << " MC particles";

        // Send the mc particle information
        auto mc_particle_message = std::make_shared<MCParticleMessage>(std::move(mc_particles[detector]), detector);
        messenger_->dispatchMessage(this, mc_particle_message);

        if(!deposits[detector].empty()) {
            // Assign MCParticles:
            for(size_t i = 0; i < deposits[detector].size(); ++i) {
                deposits[detector].at(i).setMCParticle(
                    &mc_particle_message->getData().at(particles_to_deposits[detector].at(i)));
            }

            LOG(DEBUG) << "Detector " << detector->getName() << " has " << deposits[detector].size() << " deposits";
            // Create a new charge deposit message
            auto deposit_message = std::make_shared<DepositedChargeMessage>(std::move(deposits[detector]), detector);

            // Dispatch the message
            messenger_->dispatchMessage(this, deposit_message);
        }
    }
}

void DepositionReaderModule::read_root(unsigned int event_num,
                                       DepositMap& deposits,
                                       ParticleMap& mc_particles,
                                       ParticleRelationMap& particles_to_deposits) {

    // FIXME doesn't work, tree has more entries than events
    if(event_num >= tree_reader_->GetEntries(false)) {
        throw EndOfRunException("Requesting end of run because TTree only contains data for " + std::to_string(event_num) +
                                " events");
    }

    do {
        // Separate individual events
        if(static_cast<unsigned int>(*event_->Get()) > event_num) {
            break;
        }

        // Read detector name
        // NOTE volume_->GetSize() is the full length, but we only need five characters:
        auto volume = std::string(static_cast<char*>(volume_->GetAddress()), 5);
        auto detectors = geo_manager_->getDetectors();
        auto pos = std::find_if(detectors.begin(), detectors.end(), [volume](const std::shared_ptr<Detector>& d) {
            return d->getName() == volume;
        });
        if(pos == detectors.end()) {
            LOG(WARNING) << "Did not find detector \"" << volume << "\" in the current simulation";
            continue;
        }

        // Assign detector
        auto detector = (*pos);

        // Read position of energy deposit:
        auto global_deposit_position =
            ROOT::Math::XYZPoint(Units::get(*px_->Get(), "m"), Units::get(*py_->Get(), "m"), Units::get(*pz_->Get(), "m"));
        auto deposit_position = detector->getLocalPosition(global_deposit_position);

        if(!detector->isWithinSensor(deposit_position)) {
            LOG(WARNING) << "Found deposition outside sensor at " << Units::display(deposit_position, {"mm", "um"})
                         << ". Skipping.";
            continue;
        }

        // Calculate number of electron hole pairs produced, taking into acocunt fluctuations between ionization and lattice
        // excitations via the Fano factor. We assume Gaussian statistics here.
        auto mean_charge = static_cast<unsigned int>(Units::get(*edep_->Get(), "MeV") / charge_creation_energy_);
        std::normal_distribution<double> charge_fluctuation(mean_charge, std::sqrt(mean_charge * fano_factor_));
        auto charge = charge_fluctuation(random_generator_);

        LOG(DEBUG) << "Found deposition of " << charge << " e/h pairs inside sensor at "
                   << Units::display(deposit_position, {"mm", "um"}) << " in volume " << volume;

        // FIXME time is not used, seems to be total run time?

        // MCParticle:
        mc_particles[detector].emplace_back(deposit_position,
                                            global_deposit_position,
                                            deposit_position,
                                            global_deposit_position,
                                            *pdg_code_->Get(),
                                            Units::get(0, "ns"));

        // Deposit electron
        deposits[detector].emplace_back(
            deposit_position, global_deposit_position, CarrierType::ELECTRON, charge, Units::get(0, "ns"));
        particles_to_deposits[detector].push_back(mc_particles[detector].size() - 1);

        // Deposit hole
        deposits[detector].emplace_back(
            deposit_position, global_deposit_position, CarrierType::HOLE, charge, Units::get(0, "ns"));
        particles_to_deposits[detector].push_back(mc_particles[detector].size() - 1);
    } while(tree_reader_->Next());
}

void DepositionReaderModule::read_csv(unsigned int event,
                                      DepositMap& deposits,
                                      ParticleMap& mc_particles,
                                      ParticleRelationMap& particles_to_deposits) {

    // Read input file line-by-line:
    std::string line;
    while(std::getline(*input_file_, line)) {
        // Trim whitespaces at beginning and end of line:
        line = allpix::trim(line);

        // Ignore empty lines or comments
        if(line.empty() || line.front() == '#') {
            continue;
        }

        std::stringstream ls(line);

        std::string tmp;
        unsigned int event_read;
        // Event separator:
        if(line.front() == 'E') {
            ls >> tmp >> event_read;
            if(event_read + 1 > event) {
                break;
            }
            continue;
        }

        LOG(TRACE) << "Input: " << line;

        int pdg_code;
        double time, edep, px, py, pz;
        std::string volume;
        ls >> pdg_code >> tmp >> time >> tmp >> edep >> tmp >> px >> tmp >> py >> tmp >> pz >> tmp >> volume;

        // Select the detector name from this:
        volume = volume.substr(0, 5);
        auto detectors = geo_manager_->getDetectors();
        auto pos = std::find_if(detectors.begin(), detectors.end(), [volume](const std::shared_ptr<Detector>& d) {
            return d->getName() == volume;
        });
        if(pos == detectors.end()) {
            LOG(WARNING) << "Did not find detector \"" << volume << "\" in the current simulation";
            continue;
        }

        // Assign detector
        auto detector = (*pos);

        // Calculate the charge deposit at a local position, shift by half matrix in x and y:
        auto deposit_position = ROOT::Math::XYZPoint(
            px + detector->getModel()->getSensorSize().x() / 2, py + detector->getModel()->getSensorSize().y() / 2, pz);

        if(!detector->isWithinSensor(deposit_position)) {
            LOG(WARNING) << "Found deposition outside sensor at " << Units::display(deposit_position, {"mm", "um"})
                         << ". Skipping.";
            continue;
        }

        // Calculate number of electron hole pairs produced, taking into acocunt fluctuations between ionization and lattice
        // excitations via the Fano factor. We assume Gaussian statistics here.
        auto mean_charge = static_cast<unsigned int>(Units::get(edep, "keV") / charge_creation_energy_);
        std::normal_distribution<double> charge_fluctuation(mean_charge, std::sqrt(mean_charge * fano_factor_));
        auto charge = charge_fluctuation(random_generator_);

        LOG(DEBUG) << "Found deposition of " << charge << " e/h pairs inside sensor at "
                   << Units::display(deposit_position, {"mm", "um"}) << " in volume " << volume;

        auto global_deposit_position = detector->getGlobalPosition(deposit_position);

        // MCParticle:
        mc_particles[detector].emplace_back(deposit_position,
                                            global_deposit_position,
                                            deposit_position,
                                            global_deposit_position,
                                            pdg_code,
                                            Units::get(time, "ns"));

        // Deposit electron
        deposits[detector].emplace_back(
            deposit_position, global_deposit_position, CarrierType::ELECTRON, charge, Units::get(time, "ns"));
        particles_to_deposits[detector].push_back(mc_particles[detector].size() - 1);

        // Deposit hole
        deposits[detector].emplace_back(
            deposit_position, global_deposit_position, CarrierType::HOLE, charge, Units::get(time, "ns"));
        particles_to_deposits[detector].push_back(mc_particles[detector].size() - 1);
    }
}
