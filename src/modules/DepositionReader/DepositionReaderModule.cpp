/**
 * @file
 * @brief Implementation of DepositionReader module
 *
 * @copyright Copyright (c) 2019-2020 CERN and the Allpix Squared authors.
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

    // Get the creation energy for charge (default is silicon electron hole pair energy)
    charge_creation_energy_ = config_.get<double>("charge_creation_energy", Units::get(3.64, "eV"));
    fano_factor_ = config_.get<double>("fano_factor", 0.115);
    volume_chars_ = config_.get<size_t>("detector_name_chars", 0);
}

void DepositionReaderModule::init() {

    // Check which file type we want to read:
    file_model_ = config_.get<std::string>("model");
    std::transform(file_model_.begin(), file_model_.end(), file_model_.begin(), ::tolower);
    if(file_model_ == "csv") {
        // Open the file with the objects
        auto file_path = config_.getPathWithExtension("file_name", "csv", true);
        input_file_ = std::make_unique<std::ifstream>(file_path);
        if(!input_file_->is_open()) {
            throw InvalidValueError(config_, "file_name", "could not open input file");
        }
    } else if(file_model_ == "root") {
        auto file_path = config_.getPathWithExtension("file_name", "root", true);
        input_file_root_ = std::make_unique<TFile>(file_path.c_str(), "READ");
        if(!input_file_root_->IsOpen()) {
            throw InvalidValueError(config_, "file_name", "could not open input file");
        }
        input_file_root_->cd();
        auto tree = config_.get<std::string>("tree_name");
        tree_reader_ = std::make_shared<TTreeReader>(tree.c_str(), input_file_root_.get());
        if(tree_reader_->GetEntryStatus() == TTreeReader::kEntryNoTree) {
            throw InvalidValueError(config_, "tree_name", "could not open tree");
        }
        LOG(INFO) << "Initialized tree reader for tree " << tree << ", found " << tree_reader_->GetEntries(false)
                  << " entries";

        event_ = std::make_shared<TTreeReaderValue<int>>(*tree_reader_, "event");
        edep_ = std::make_shared<TTreeReaderValue<double>>(*tree_reader_, "energy.Edep");
        time_ = std::make_shared<TTreeReaderValue<double>>(*tree_reader_, "time");
        px_ = std::make_shared<TTreeReaderValue<double>>(*tree_reader_, "position.x");
        py_ = std::make_shared<TTreeReaderValue<double>>(*tree_reader_, "position.y");
        pz_ = std::make_shared<TTreeReaderValue<double>>(*tree_reader_, "position.z");
        volume_ = std::make_shared<TTreeReaderArray<char>>(*tree_reader_, "detector");
        pdg_code_ = std::make_shared<TTreeReaderValue<int>>(*tree_reader_, "PDG_code");

        track_id_ = std::make_shared<TTreeReaderValue<int>>(*tree_reader_, "track_id");
        parent_id_ = std::make_shared<TTreeReaderValue<int>>(*tree_reader_, "parent_id");

        tree_reader_->Next();
    } else {
        throw InvalidValueError(config_, "model", "only models 'root' and 'csv' are currently supported");
    }
}

void DepositionReaderModule::run(unsigned int event) {

    // Set of deposited charges in this event
    std::map<std::shared_ptr<Detector>, std::vector<DepositedCharge>> deposits;
    std::map<std::shared_ptr<Detector>, std::vector<MCParticle>> mc_particles;
    std::map<std::shared_ptr<Detector>, std::vector<int>> particles_to_deposits;
    std::map<std::shared_ptr<Detector>, std::map<int, size_t>> track_id_to_mcparticle;

    LOG(DEBUG) << "Start reading event " << event;

    do {
        bool read_status = false;
        ROOT::Math::XYZPoint global_deposit_position;
        std::string volume;
        double energy, time;
        int pdg_code, track_id, parent_id;

        if(file_model_ == "csv") {
            read_status = read_csv(event, volume, global_deposit_position, time, energy, pdg_code, track_id, parent_id);
        } else if(file_model_ == "root") {
            read_status = read_root(event, volume, global_deposit_position, time, energy, pdg_code, track_id, parent_id);
        }

        if(!read_status) {
            break;
        }

        auto detectors = geo_manager_->getDetectors();
        auto pos = std::find_if(detectors.begin(), detectors.end(), [volume](const std::shared_ptr<Detector>& d) {
            return d->getName() == volume;
        });
        if(pos == detectors.end()) {
            LOG(TRACE) << "Ignored detector \"" << volume << "\", not found in current simulation";
            continue;
        }
        // Assign detector
        auto detector = (*pos);
        LOG(DEBUG) << "Found detector \"" << detector->getName() << "\"";

        auto deposit_position = detector->getLocalPosition(global_deposit_position);
        if(!detector->isWithinSensor(deposit_position)) {
            LOG(WARNING) << "Found deposition outside sensor at " << Units::display(deposit_position, {"mm", "um"})
                         << ", global " << Units::display(global_deposit_position, {"mm", "um"}) << ". Skipping.";
            continue;
        }

        // Calculate number of electron hole pairs produced, taking into acocunt fluctuations between ionization and lattice
        // excitations via the Fano factor. We assume Gaussian statistics here.
        auto mean_charge = static_cast<unsigned int>(energy / charge_creation_energy_);
        std::normal_distribution<double> charge_fluctuation(mean_charge, std::sqrt(mean_charge * fano_factor_));
        auto charge = charge_fluctuation(random_generator_);

        LOG(DEBUG) << "Found deposition of " << charge << " e/h pairs inside sensor at "
                   << Units::display(deposit_position, {"mm", "um"}) << " in detector " << detector->getName() << ", global "
                   << Units::display(global_deposit_position, {"mm", "um"}) << ", particleID " << pdg_code;

        // MCParticle:
        if(track_id_to_mcparticle[detector].find(track_id) == track_id_to_mcparticle[detector].end()) {
            // We have not yet seen this MCParticle, let's store it and meep track of the track id
            LOG(DEBUG) << "Adding new MCParticle, track id " << track_id << ", PDG code " << pdg_code;
            mc_particles[detector].emplace_back(
                deposit_position, global_deposit_position, deposit_position, global_deposit_position, pdg_code, time);
            track_id_to_mcparticle[detector][track_id] = (mc_particles[detector].size() - 1);

            // Check if we know the parent - and set it:
            auto parent = track_id_to_mcparticle[detector].find(parent_id);
            if(parent != track_id_to_mcparticle[detector].end()) {
                LOG(DEBUG) << "Adding parent relation to MCParticle with track id " << parent_id;
                mc_particles[detector].back().setParent(&mc_particles[detector].at(parent->second));
            } else {
                LOG(DEBUG) << "Parent MCParticle is unknown, parent id " << parent_id;
            }
        } else {
            LOG(DEBUG) << "Found MCParticle with track id " << track_id;
        }

        // Deposit electron
        deposits[detector].emplace_back(deposit_position, global_deposit_position, CarrierType::ELECTRON, charge, time);
        particles_to_deposits[detector].push_back(track_id);

        // Deposit hole
        deposits[detector].emplace_back(deposit_position, global_deposit_position, CarrierType::HOLE, charge, time);
        particles_to_deposits[detector].push_back(track_id);
    } while(true);

    LOG(INFO) << "Finished reading event " << event;

    // Loop over all known detectors and dispatch messages for them
    for(const auto& detector : geo_manager_->getDetectors()) {
        LOG(DEBUG) << "Detector " << detector->getName() << " has " << mc_particles[detector].size() << " MC particles";

        // Send the mc particle information
        auto mc_particle_message = std::make_shared<MCParticleMessage>(std::move(mc_particles[detector]), detector);
        messenger_->dispatchMessage(this, mc_particle_message);

        if(!deposits[detector].empty()) {
            // Assign MCParticles:
            for(size_t i = 0; i < deposits[detector].size(); ++i) {
                deposits[detector].at(i).setMCParticle(&mc_particle_message->getData().at(
                    track_id_to_mcparticle[detector].at(particles_to_deposits[detector].at(i))));
            }

            LOG(DEBUG) << "Detector " << detector->getName() << " has " << deposits[detector].size() << " deposits";
            // Create a new charge deposit message
            auto deposit_message = std::make_shared<DepositedChargeMessage>(std::move(deposits[detector]), detector);

            // Dispatch the message
            messenger_->dispatchMessage(this, deposit_message);
        }
    }
}

bool DepositionReaderModule::read_root(unsigned int event_num,
                                       std::string& volume,
                                       ROOT::Math::XYZPoint& position,
                                       double& time,
                                       double& energy,
                                       int& pdg_code,
                                       int& track_id,
                                       int& parent_id) {
    using namespace ROOT::Math;

    auto status = tree_reader_->GetEntryStatus();

#ifdef ROOT_TTREEREADER_VERBOSE
    auto status_message = std::string(tree_reader_->fgEntryStatusText[status]);
#else
    auto status_message = std::string("unkown error");
#endif

    if(status != TTreeReader::kEntryValid) {
        throw EndOfRunException("Requesting end of run because TTree reported status \"" + status_message + "\"");
    } else if(status != TTreeReader::kEntryValid) {
        throw ModuleError("Problem reading from tree: \"" + status_message + "\"");
    }

    // Separate individual events
    if(static_cast<unsigned int>(*event_->Get()) > event_num) {
        return false;
    }

    // Read detector name
    // NOTE volume_->GetSize() is the full length, we might want to cut only part of the name
    auto length = (volume_chars_ != 0 ? std::min(volume_chars_, volume_->GetSize()) : volume_->GetSize());
    volume = std::string(static_cast<char*>(volume_->GetAddress()), length);
    position = XYZPoint(Units::get(*px_->Get(), "m"), Units::get(*py_->Get(), "m"), Units::get(*pz_->Get(), "m"));
    time = Units::get(*time_->Get(), "ns");
    energy = Units::get(*edep_->Get(), "MeV");
    pdg_code = (*pdg_code_->Get());
    track_id = (*track_id_->Get());
    parent_id = (*parent_id_->Get());
    return (tree_reader_->Next());
}

bool DepositionReaderModule::read_csv(unsigned int event_num,
                                      std::string& volume,
                                      ROOT::Math::XYZPoint& position,
                                      double& time,
                                      double& energy,
                                      int& pdg_code,
                                      int&,
                                      int&) {

    std::string line, tmp;
    do {
        LOG(DEBUG) << "here1";
        // Read input file line-by-line and trim whitespaces at beginning and end:
        std::getline(*input_file_, line);
        line = allpix::trim(line);
        LOG(DEBUG) << "Input: " << line;

        // Check for event header:
        if(line.front() == 'E') {
            std::stringstream lse(line);
            unsigned int event_read;
            lse >> tmp >> event_read;
            if(event_read + 1 > event_num) {
                return false;
            }
            LOG(DEBUG) << "Parsed event, continuing";
            continue;
        }
    } while(line.empty() || line.front() == '#' || line.front() == 'E');

    std::stringstream ls(line);
    double t, edep, px, py, pz;
    ls >> pdg_code >> tmp >> t >> tmp >> edep >> tmp >> px >> tmp >> py >> tmp >> pz >> tmp >> volume;

    // Select the detector name from this:
    if(volume_chars_ != 0) {
        volume = volume.substr(0, std::min(volume_chars_, volume.size()));
    }

    // Calculate the charge deposit at a global position
    position = ROOT::Math::XYZPoint(px, py, pz);
    time = Units::get(time, "s");
    energy = Units::get(edep, "keV");

    return true;
}
