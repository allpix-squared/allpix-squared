/**
 * @file
 * @brief Implementation of DepositionReader module
 *
 * @copyright Copyright (c) 2019-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "DepositionReaderModule.hpp"

#include <string>
#include <utility>

#include "core/utils/distributions.h"
#include "core/utils/log.h"
#include "physics/MaterialProperties.hpp"

using namespace allpix;

DepositionReaderModule::DepositionReaderModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager)
    : SequentialModule(config), geo_manager_(geo_manager), messenger_(messenger) {
    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();

    config_.setDefault<size_t>("detector_name_chars", 0);
    config_.setDefault<std::string>("unit_length", "mm");
    config_.setDefault<std::string>("unit_time", "ns");
    config_.setDefault<std::string>("unit_energy", "MeV");
    config_.setDefault<bool>("require_sequential_events", true);
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

    file_model_ = config_.get<FileModel>("model");
    volume_chars_ = config_.get<size_t>("detector_name_chars");

    unit_length_ = config_.get<std::string>("unit_length");
    unit_time_ = config_.get<std::string>("unit_time");
    unit_energy_ = config_.get<std::string>("unit_energy");

    require_sequential_events_ = config_.get<bool>("require_sequential_events");
    time_available_ = config_.get<bool>("assign_timestamps");
    create_mcparticles_ = config.get<bool>("create_mcparticles");

    output_plots_ = config_.get<bool>("output_plots");
}

void DepositionReaderModule::initialize() {

    if(!time_available_) {
        LOG(WARNING) << "No time information provided, all energy deposition will be assigned to t = 0";
    }
    if(!create_mcparticles_) {
        LOG(WARNING) << "No MCParticle objects will be produced";
    }

    // Check which file type we want to read:
    if(file_model_ == FileModel::CSV) {
        // Open the file with the objects
        auto file_path = config_.getPathWithExtension("file_name", "csv", true);
        input_file_ = std::make_unique<std::ifstream>(file_path);
        if(!input_file_->is_open()) {
            throw InvalidValueError(config_, "file_name", "could not open input file");
        }
    } else if(file_model_ == FileModel::ROOT) {
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
    }

    // If requested, prepare output plots
    if(output_plots_) {
        LOG(TRACE) << "Creating output plots";
        for(auto& detector : geo_manager_->getDetectors()) {

            // Plot axis are in kilo electrons - convert from framework units!
            int maximum = static_cast<int>(Units::convert(config_.get<int>("output_plots_scale"), "ke"));
            int nbins = 5 * maximum;

            // Create histograms if needed
            std::string plot_name = "deposited_charge_" + detector->getName();
            charge_per_event_[detector] = CreateHistogram<TH1D>(
                plot_name.c_str(), "deposited charge per event;deposited charge [ke];events", nbins, 0, maximum);
        }
    }

    // Calculate ionization energies and Fano factors:
    for(auto& detector : geo_manager_->getDetectors()) {
        auto model = detector->getModel();
        charge_creation_energy_[detector] =
            (config_.has("charge_creation_energy") ? config_.get<double>("charge_creation_energy")
                                                   : allpix::ionization_energies[model->getSensorMaterial()]);
        fano_factor_[detector] = (config_.has("fano_factor") ? config_.get<double>("fano_factor")
                                                             : allpix::fano_factors[model->getSensorMaterial()]);
        LOG(DEBUG) << "Detector " << detector->getName() << " uses charge creation energy "
                   << Units::display(charge_creation_energy_[detector], "eV") << " and Fano factor "
                   << fano_factor_[detector];
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

void DepositionReaderModule::run(Event* event) {
    auto event_num = event->number;

    // Set of deposited charges in this event
    std::map<std::shared_ptr<Detector>, std::vector<ROOT::Math::XYZPoint>> deposit_position;
    std::map<std::shared_ptr<Detector>, std::vector<unsigned int>> deposit_charge;
    std::map<std::shared_ptr<Detector>, std::vector<double>> deposit_time;

    std::map<std::shared_ptr<Detector>, std::vector<ROOT::Math::XYZPoint>> mc_particle_start;
    std::map<std::shared_ptr<Detector>, std::vector<ROOT::Math::XYZPoint>> mc_particle_end;
    std::map<std::shared_ptr<Detector>, std::vector<int>> mc_particle_code;
    std::map<std::shared_ptr<Detector>, std::vector<double>> mc_particle_time;
    std::map<std::shared_ptr<Detector>, std::vector<int>> mc_particle_parent;
    std::map<std::shared_ptr<Detector>, std::vector<unsigned int>> mc_particle_charge;

    std::map<std::shared_ptr<Detector>, std::vector<int>> particles_to_deposits;
    std::map<std::shared_ptr<Detector>, std::map<int, size_t>> track_id_to_mcparticle;

    LOG(DEBUG) << "Start reading event " << event_num;
    int64_t curr_event_id = -1;
    bool end_of_run = false;
    std::string eof_message;

    do {
        bool read_status = false;
        ROOT::Math::XYZPoint global_position;
        std::string volume;
        double energy = NAN, time = NAN;
        int pdg_code = 0, track_id = 0, parent_id = 0;

        try {
            if(file_model_ == FileModel::CSV) {
                read_status = read_csv(event_num, volume, global_position, time, energy, pdg_code, track_id, parent_id);
            } else if(file_model_ == FileModel::ROOT) {
                read_status = read_root(
                    event_num, curr_event_id, volume, global_position, time, energy, pdg_code, track_id, parent_id);
            }
        } catch(EndOfRunException& e) {
            end_of_run = true;
            eof_message = e.what();
        }

        if(!read_status || end_of_run) {
            break;
        }

        // Trim detector name if requested:
        if(volume_chars_ != 0) {
            volume = volume.substr(0, std::min(volume_chars_, volume.size()));
            LOG(TRACE) << "Truncated detector name: " << volume;
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

        auto local_position = detector->getLocalPosition(global_position);
        if(!detector->getModel()->isWithinSensor(local_position)) {
            LOG(WARNING) << "Found deposition outside sensor at " << Units::display(local_position, {"mm", "um"})
                         << ", global " << Units::display(global_position, {"mm", "um"}) << ". Skipping.";
            continue;
        }

        // Calculate number of electron hole pairs produced, taking into account fluctuations between ionization and lattice
        // excitations via the Fano factor. We assume Gaussian statistics here.
        auto mean_charge = energy / charge_creation_energy_[detector];
        allpix::normal_distribution<double> charge_fluctuation(mean_charge, std::sqrt(mean_charge * fano_factor_[detector]));
        auto charge = static_cast<unsigned int>(charge_fluctuation(event->getRandomEngine()));

        LOG(DEBUG) << "Found deposition of " << charge << " e/h pairs inside sensor at "
                   << Units::display(local_position, {"mm", "um"}) << " in detector " << detector->getName() << ", global "
                   << Units::display(global_position, {"mm", "um"}) << ", particleID " << pdg_code << ", time "
                   << Units::display(time, {"ns", "us"});

        // Store information about deposited charge carriers
        deposit_position[detector].push_back(global_position);
        deposit_charge[detector].push_back(charge);
        deposit_time[detector].push_back(time);

        // No MCParticle creation requested:
        if(!create_mcparticles_) {
            continue;
        }

        // MCParticle:
        auto iter = track_id_to_mcparticle[detector].find(track_id);
        if(iter == track_id_to_mcparticle[detector].end()) {
            // We have not yet seen this MCParticle, let's store it and keep track of the track id
            LOG(DEBUG) << "Adding new MCParticle, track id " << track_id << ", PDG code " << pdg_code;
            mc_particle_start[detector].push_back(global_position);
            mc_particle_end[detector].push_back(global_position);
            mc_particle_time[detector].push_back(time);
            mc_particle_code[detector].push_back(pdg_code);
            mc_particle_parent[detector].push_back(parent_id);
            mc_particle_charge[detector].push_back(charge);
            track_id_to_mcparticle[detector][track_id] = (mc_particle_start[detector].size() - 1);
        } else {
            LOG(DEBUG) << "Found MCParticle with track id " << track_id << ", updating position";
            mc_particle_end[detector].at(iter->second) = global_position;
            mc_particle_charge[detector].at(iter->second) += charge;
        }

        particles_to_deposits[detector].push_back(track_id);
    } while(true);

    LOG(INFO) << "Finished reading event " << event;

    double time_reference = 0;

    // Loop over all known detectors and dispatch messages for them
    for(const auto& detector : geo_manager_->getDetectors()) {

        if(!mc_particle_time[detector].empty()) {
            time_reference = *std::min_element(mc_particle_time[detector].begin(), mc_particle_time[detector].end());
            LOG(DEBUG) << "Earliest MCParticle arrived on detector " << detector->getName() << " at "
                       << Units::display(time_reference, {"ns", "ps"}) << " global";
        }

        auto mc_particle_size = mc_particle_start[detector].size();
        std::vector<MCParticle> mc_particles;
        mc_particles.reserve(mc_particle_size);

        for(size_t i = 0; i < mc_particle_size; i++) {
            auto start_global = mc_particle_start[detector].at(i);
            auto start_local = detector->getLocalPosition(start_global);
            auto end_global = mc_particle_end[detector].at(i);
            auto end_local = detector->getLocalPosition(end_global);

            auto pdg_code = mc_particle_code[detector].at(i);
            auto time = mc_particle_time[detector].at(i);

            mc_particles.emplace_back(
                start_local, start_global, end_local, end_global, pdg_code, time - time_reference, time);
            // Count electrons and holes:
            mc_particles.back().setTotalDepositedCharge(2 * mc_particle_charge[detector].at(i));
        }

        for(size_t i = 0; i < mc_particle_size; i++) {
            // Check if we know the parent - and set it:
            auto parent_id = mc_particle_parent[detector].at(i);
            auto parent = track_id_to_mcparticle[detector].find(parent_id);
            if(parent == track_id_to_mcparticle[detector].end()) {
                LOG(DEBUG) << "Parent MCParticle is unknown, parent track id " << parent_id;
            } else if(i == parent->second) {
                LOG(DEBUG) << "Parent MCParticle is same as current particle, not adding relation";
            } else {
                LOG(DEBUG) << "Adding parent relation to MCParticle with track id " << parent_id;
                mc_particles.at(i).setParent(&mc_particles.at(parent->second));
            }
        }

        // Send the mc particle information if available
        bool has_mcparticles = !mc_particles.empty();
        auto mc_particle_message = std::make_shared<MCParticleMessage>(std::move(mc_particles), detector);
        if(has_mcparticles) {
            messenger_->dispatchMessage(this, mc_particle_message, event);
        }

        if(!deposit_position[detector].empty()) {
            std::map<std::shared_ptr<Detector>, std::vector<DepositedCharge>> deposits;
            double total_deposits = 0;

            for(size_t i = 0; i < deposit_position[detector].size(); i++) {
                auto global_position = deposit_position[detector].at(i);
                auto local_position = detector->getLocalPosition(global_position);
                auto time = deposit_time[detector].at(i);
                auto charge = deposit_charge[detector].at(i);
                total_deposits += 2 * charge;

                // Deposit electron
                deposits[detector].emplace_back(
                    local_position, global_position, CarrierType::ELECTRON, charge, time - time_reference, time);

                if(create_mcparticles_) {
                    deposits[detector].back().setMCParticle(&mc_particle_message->getData().at(
                        track_id_to_mcparticle[detector].at(particles_to_deposits[detector].at(i))));
                }

                // Deposit hole
                deposits[detector].emplace_back(
                    local_position, global_position, CarrierType::HOLE, charge, time - time_reference, time);
                if(create_mcparticles_) {
                    deposits[detector].back().setMCParticle(&mc_particle_message->getData().at(
                        track_id_to_mcparticle[detector].at(particles_to_deposits[detector].at(i))));
                }
            }

            // Create a new charge deposit message
            LOG(DEBUG) << "Detector " << detector->getName() << " has " << deposits[detector].size() << " deposits";
            auto deposit_message = std::make_shared<DepositedChargeMessage>(std::move(deposits[detector]), detector);

            // Dispatch the message
            messenger_->dispatchMessage(this, deposit_message, event);

            // Fill output plots if requested:
            if(output_plots_) {
                double charge = static_cast<double>(Units::convert(total_deposits, "ke"));
                charge_per_event_[detector]->Fill(charge);
            }
        }
    }

    // Request end-of-run since we don't have events anymore
    if(end_of_run) {
        throw EndOfRunException(eof_message);
    }
}

void DepositionReaderModule::finalize() {
    if(output_plots_) {
        // Write histograms
        LOG(TRACE) << "Writing output plots to file";
        for(auto& plot : charge_per_event_) {
            plot.second->Write();
        }
    }
}
bool DepositionReaderModule::read_root(uint64_t event_num,
                                       int64_t& curr_event_id,
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

    if(require_sequential_events_) {
        // sequential read, return if eventID is larger than allpix-squared event number
        if(static_cast<uint64_t>(*event_->Get()) > event_num - 1) {
            return false;
        }
    } else {
        // non-sequential read, return if eventID changes (requires events to be in blocks)
        if(curr_event_id == -1) {
            // first entry in event, save eventID for later
            curr_event_id = *event_->Get();
            LOG(TRACE) << "Read eventID " << curr_event_id;
        } else {
            // check if eventID changed compared to last entry, if yes reset curr_event_id
            if(*event_->Get() != curr_event_id) {
                curr_event_id = -1;
                return false;
            }
        }
    }

    // Read detector name
    volume = std::string(static_cast<char*>(volume_->GetAddress()));

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

bool DepositionReaderModule::read_csv(uint64_t event_num,
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
            uint64_t event_read = 0;
            lse >> tmp >> event_read;
            if(event_read + 1 > event_num) {
                return false;
            }
            LOG(DEBUG) << "Parsed header of event " << event_read << ", continuing";
            continue;
        }
    } while(line.empty() || line.front() == '#' || line.front() == 'E');

    std::istringstream ls(line);
    double px = NAN, py = NAN, pz = NAN;

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

    // Calculate the charge deposit at a global position and convert the proper units
    position =
        ROOT::Math::XYZPoint(Units::get(px, unit_length_), Units::get(py, unit_length_), Units::get(pz, unit_length_));
    time = (time_available_ ? Units::get(time, unit_time_) : 0);
    energy = Units::get(energy, unit_energy_);

    return true;
}
