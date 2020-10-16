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

    config_.setDefault<double>("charge_creation_energy", Units::get(3.64, "eV"));
    config_.setDefault<double>("fano_factor", 0.115);
    config_.setDefault<size_t>("detector_name_chars", 0);
    config_.setDefault<std::string>("unit_length", "mm");
    config_.setDefault<std::string>("unit_time", "ns");
    config_.setDefault<std::string>("unit_energy", "MeV");
    config_.setDefault<bool>("assign_timestamps", true);
    config_.setDefault<bool>("create_mcparticles", true);

    config_.setDefaultArray<std::string>("branch_names",
                                         {"event",
                                          "energy",
                                          "time",
                                          "position.x",
                                          "position.y",
                                          "position.z",
                                          "detector",
                                          "pdg_code",
                                          "track_id",
                                          "parent_id"});

    config_.setDefault<bool>("output_plots", false);
    config_.setDefault<int>("output_plots_scale", Units::get(100, "ke"));

    // Get the creation energy for charge (default is silicon electron hole pair energy)
    charge_creation_energy_ = config_.get<double>("charge_creation_energy");
    fano_factor_ = config_.get<double>("fano_factor");
    volume_chars_ = config_.get<size_t>("detector_name_chars");

    unit_length_ = config_.get<std::string>("unit_length");
    unit_time_ = config_.get<std::string>("unit_time");
    unit_energy_ = config_.get<std::string>("unit_energy");

    time_available_ = config_.get<bool>("assign_timestamps");
    create_mcparticles_ = config.get<bool>("create_mcparticles");
}

void DepositionReaderModule::init() {

    if(!time_available_) {
        LOG(WARNING) << "No time information provided, all energy deposition will be assigned to t = 0";
    }
    if(!create_mcparticles_) {
        LOG(ERROR) << "No MCParticle objects will be produced";
    }

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

        // Check if we have branch names configured and use the default values otherwise:
        auto branch_list = config_.getArray<std::string>("branch_names");

        // Exactly 10 branch names are required unless time or monte carlo particles are left out:
        size_t required_list_size =
            10 - static_cast<size_t>(time_available_ ? 0 : 1) - static_cast<size_t>(create_mcparticles_ ? 0 : 2);
        if(branch_list.size() != required_list_size) {
            throw InvalidValueError(config_,
                                    "branch_names",
                                    "With the current configuration, this parameter requires exactly " +
                                        std::to_string(required_list_size) + " entries, one for each branch to be read");
        }

        // Convert list to map for easier lookup:
        size_t it = (time_available_ ? 3 : 2);
        std::map<std::string, std::string> branches = {{"event", branch_list.at(0)},
                                                       {"energy", branch_list.at(1)},
                                                       {"px", branch_list.at(it++)},
                                                       {"py", branch_list.at(it++)},
                                                       {"pz", branch_list.at(it++)},
                                                       {"volume", branch_list.at(it++)},
                                                       {"pdg", branch_list.at(it++)}};
        if(time_available_) {
            branches["time"] = branch_list.at(2);
        }
        if(create_mcparticles_) {
            branches["track_id"] = branch_list.at(it++);
            branches["parent_id"] = branch_list.at(it++);
        }

        LOG(DEBUG) << "List of configured branches and their names:";
        for(const auto& branch : branches) {
            LOG(DEBUG) << branch.first << ": \"" << branch.second << "\"";
        }

        // Set up branch pointers
        create_tree_reader(event_, branches.at("event"));
        create_tree_reader(edep_, branches.at("energy"));
        if(time_available_) {
            create_tree_reader(time_, branches.at("time"));
        }
        create_tree_reader(px_, branches.at("px"));
        create_tree_reader(py_, branches.at("py"));
        create_tree_reader(pz_, branches.at("pz"));
        create_tree_reader(volume_, branches.at("volume"));
        create_tree_reader(pdg_code_, branches.at("pdg"));
        if(create_mcparticles_) {
            create_tree_reader(track_id_, branches.at("track_id"));
            create_tree_reader(parent_id_, branches.at("parent_id"));
        }

        // Advance to first entry of the tree:
        tree_reader_->Next();

        // Only after loading the first entry we can actually check the branch status:
        check_tree_reader(event_);
        check_tree_reader(edep_);
        if(time_available_) {
            check_tree_reader(time_);
        }
        check_tree_reader(px_);
        check_tree_reader(py_);
        check_tree_reader(pz_);
        check_tree_reader(volume_);
        check_tree_reader(pdg_code_);
        if(create_mcparticles_) {
            check_tree_reader(track_id_);
            check_tree_reader(parent_id_);
        }

    } else {
        throw InvalidValueError(config_, "model", "only models 'root' and 'csv' are currently supported");
    }

    for(auto& detector : geo_manager_->getDetectors()) {
        // If requested, prepare output plots
        if(config_.get<bool>("output_plots")) {
            LOG(TRACE) << "Creating output plots";

            // Plot axis are in kilo electrons - convert from framework units!
            int maximum = static_cast<int>(Units::convert(config_.get<int>("output_plots_scale"), "ke"));
            int nbins = 5 * maximum;

            // Create histograms if needed
            std::string plot_name = "deposited_charge_" + detector->getName();
            charge_per_event_[detector->getName()] =
                new TH1D(plot_name.c_str(), "deposited charge per event;deposited charge [ke];events", nbins, 0, maximum);
        }
    }
}

template <typename T>
void DepositionReaderModule::create_tree_reader(std::shared_ptr<T>& branch_ptr, const std::string& name) {
    branch_ptr = std::make_shared<T>(*tree_reader_, name.c_str());
}

template <typename T> void DepositionReaderModule::check_tree_reader(std::shared_ptr<T> branch_ptr) {
    if(branch_ptr->GetSetupStatus() < 0) {
        throw InvalidValueError(
            config_, "branch_names", "Could not read branch \"" + std::string(branch_ptr->GetBranchName()) + "\"");
    }
}

void DepositionReaderModule::run(unsigned int event) {

    // Set of deposited charges in this event
    std::map<std::shared_ptr<Detector>, std::vector<DepositedCharge>> deposits;
    std::map<std::shared_ptr<Detector>, std::vector<MCParticle>> mc_particles;
    std::map<std::shared_ptr<Detector>, std::vector<int>> particles_to_deposits;
    std::map<std::shared_ptr<Detector>, std::map<int, size_t>> track_id_to_mcparticle;

    LOG(DEBUG) << "Start reading event " << event;
    bool end_of_run = false;
    std::string eof_message;

    do {
        bool read_status = false;
        ROOT::Math::XYZPoint global_deposit_position;
        std::string volume;
        double energy, time;
        int pdg_code, track_id, parent_id;

        try {
            if(file_model_ == "csv") {
                read_status = read_csv(event, volume, global_deposit_position, time, energy, pdg_code, track_id, parent_id);
            } else if(file_model_ == "root") {
                read_status = read_root(event, volume, global_deposit_position, time, energy, pdg_code, track_id, parent_id);
            }
        } catch(EndOfRunException& e) {
            end_of_run = true;
            eof_message = e.what();
        }

        if(!read_status || end_of_run) {
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

        // Calculate number of electron hole pairs produced, taking into account fluctuations between ionization and lattice
        // excitations via the Fano factor. We assume Gaussian statistics here.
        auto mean_charge = static_cast<unsigned int>(energy / charge_creation_energy_);
        std::normal_distribution<double> charge_fluctuation(mean_charge, std::sqrt(mean_charge * fano_factor_));
        auto charge = charge_fluctuation(random_generator_);

        LOG(DEBUG) << "Found deposition of " << charge << " e/h pairs inside sensor at "
                   << Units::display(deposit_position, {"mm", "um"}) << " in detector " << detector->getName() << ", global "
                   << Units::display(global_deposit_position, {"mm", "um"}) << ", particleID " << pdg_code;

        // MCParticle:
        if(create_mcparticles_) {
            if(track_id_to_mcparticle[detector].find(track_id) == track_id_to_mcparticle[detector].end()) {
                // We have not yet seen this MCParticle, let's store it and keep track of the track id
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

        // Treat MCParticles
        auto mc_particle_message = std::make_shared<MCParticleMessage>(std::move(mc_particles[detector]), detector);
        if(create_mcparticles_) {
            // Send the mc particle information
            messenger_->dispatchMessage(this, mc_particle_message);
        }

        if(!deposits[detector].empty()) {
            double total_deposits = 0;

            // Assign MCParticles:
            for(size_t i = 0; i < deposits[detector].size(); ++i) {
                total_deposits += deposits[detector].at(i).getCharge();

                if(create_mcparticles_) {
                    deposits[detector].at(i).setMCParticle(&mc_particle_message->getData().at(
                        track_id_to_mcparticle[detector].at(particles_to_deposits[detector].at(i))));
                }
            }

            // Create a new charge deposit message
            LOG(DEBUG) << "Detector " << detector->getName() << " has " << deposits[detector].size() << " deposits";
            auto deposit_message = std::make_shared<DepositedChargeMessage>(std::move(deposits[detector]), detector);

            // Dispatch the message
            messenger_->dispatchMessage(this, deposit_message);

            // Fill output plots if requested:
            if(config_.get<bool>("output_plots")) {
                double charge = static_cast<double>(Units::convert(total_deposits, "ke"));
                charge_per_event_[detector->getName()]->Fill(charge);
            }
        }
    }

    // Request end-of-run since we don't have events anymore
    if(end_of_run) {
        throw EndOfRunException(eof_message);
    }
}

void DepositionReaderModule::finalize() {
    if(config_.get<bool>("output_plots")) {
        // Write histograms
        LOG(TRACE) << "Writing output plots to file";
        for(auto& plot : charge_per_event_) {
            plot.second->Write();
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

    auto status = tree_reader_->GetEntryStatus();
    if(status == TTreeReader::kEntryNotFound || status == TTreeReader::kEntryBeyondEnd) {
        throw EndOfRunException("Requesting end of run: end of tree reached");
    } else if(status != TTreeReader::kEntryValid) {
        throw EndOfRunException("Problem reading from tree, error: " + std::to_string(static_cast<int>(status)));
    }

    // Separate individual events
    if(static_cast<unsigned int>(*event_->Get()) > event_num - 1) {
        return false;
    }

    // Read detector name
    // NOTE volume_->GetSize() is the full length, we might want to cut only part of the name
    auto length = (volume_chars_ != 0 ? std::min(volume_chars_, volume_->GetSize()) : volume_->GetSize());
    volume = std::string(static_cast<char*>(volume_->GetAddress()), length);

    // Read other information, interpret in framework units:
    position = ROOT::Math::XYZPoint(
        Units::get(*px_->Get(), unit_length_), Units::get(*py_->Get(), unit_length_), Units::get(*pz_->Get(), unit_length_));

    // Attempt to read time only if available:
    time = (time_available_ ? Units::get(*time_->Get(), unit_time_) : 0);
    energy = Units::get(*edep_->Get(), unit_energy_);

    // Read PDG code and track ids
    pdg_code = (*pdg_code_->Get());
    if(create_mcparticles_) {
        track_id = (*track_id_->Get());
        parent_id = (*parent_id_->Get());
    }

    // Return and advance to next tree entry:
    tree_reader_->Next();
    return true;
}

bool DepositionReaderModule::read_csv(unsigned int event_num,
                                      std::string& volume,
                                      ROOT::Math::XYZPoint& position,
                                      double& time,
                                      double& energy,
                                      int& pdg_code,
                                      int& track_id,
                                      int& parent_id) {

    std::string line, tmp;
    do {
        // Read input file line-by-line and trim whitespaces at beginning and end:
        std::getline(*input_file_, line);
        line = allpix::trim(line);
        LOG(TRACE) << "Line read: " << line;

        // Request end of run if we reached end of file:
        if(input_file_->eof()) {
            throw EndOfRunException("Requesting end of run, CSV file only contains data for " + std::to_string(event_num) +
                                    " events");
        }

        // Check for event header:
        if(line.front() == 'E') {
            std::stringstream lse(line);
            unsigned int event_read;
            lse >> tmp >> event_read;
            if(event_read + 1 > event_num) {
                return false;
            }
            LOG(DEBUG) << "Parsed header of event " << event_read << ", continuing";
            continue;
        }
    } while(line.empty() || line.front() == '#' || line.front() == 'E');

    std::istringstream ls(line);
    double px, py, pz;

    std::getline(ls, tmp, ',');
    std::istringstream(tmp) >> pdg_code;

    if(time_available_) {
        std::getline(ls, tmp, ',');
        std::istringstream(tmp) >> time;
    }

    std::getline(ls, tmp, ',');
    std::istringstream(tmp) >> energy;

    std::getline(ls, tmp, ',');
    std::istringstream(tmp) >> px;
    std::getline(ls, tmp, ',');
    std::istringstream(tmp) >> py;
    std::getline(ls, tmp, ',');
    std::istringstream(tmp) >> pz;

    std::getline(ls, volume, ',');
    volume = allpix::trim(volume);

    if(create_mcparticles_) {
        std::getline(ls, tmp, ',');
        std::istringstream(tmp) >> track_id;
        std::getline(ls, tmp, ',');
        std::istringstream(tmp) >> parent_id;
    }

    // Select the detector name from this:
    if(volume_chars_ != 0) {
        volume = volume.substr(0, std::min(volume_chars_, volume.size()));
        LOG(TRACE) << "Truncated detector name: " << volume;
    }

    // Calculate the charge deposit at a global position and convert the proper units
    position =
        ROOT::Math::XYZPoint(Units::get(px, unit_length_), Units::get(py, unit_length_), Units::get(pz, unit_length_));
    time = (time_available_ ? Units::get(time, unit_time_) : 0);
    energy = Units::get(energy, unit_energy_);

    return true;
}
