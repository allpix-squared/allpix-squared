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
#include "objects/DepositedCharge.hpp"

using namespace allpix;

DepositionReaderModule::DepositionReaderModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager)
    : Module(config), geo_manager_(geo_manager), messenger_(messenger) {

    // Seed the random generator for Fano fluctuations with the seed received
    random_generator_.seed(getRandomSeed());
}

void DepositionReaderModule::init() {

    // Check which file type we want to read:
    auto file_model = config_.get<std::string>("model");
    if(file_model != "csv") {
        throw InvalidValueError(config_, "model", "only model 'csv' is currently supported");
    }

    // Open the file with the objects
    input_file_ = std::make_unique<std::ifstream>(config_.getPath("file_name", true));
    if(!input_file_->is_open()) {
        throw InvalidValueError(config_, "file_name", "could not open input file");
    }

    // Get the creation energy for charge (default is silicon electron hole pair energy)
    charge_creation_energy_ = config_.get<double>("charge_creation_energy", Units::get(3.64, "eV"));
    fano_factor_ = config_.get<double>("fano_factor", 0.115);
}

void DepositionReaderModule::run(unsigned int event) {

    // Set of deposited charges in this event
    std::map<std::shared_ptr<Detector>, std::vector<DepositedCharge>> deposits;
    std::map<std::shared_ptr<Detector>, std::vector<MCParticle>> mc_particles;
    std::map<std::shared_ptr<Detector>, std::vector<size_t>> particles_to_deposits;

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
        auto detector = (*pos);

        // Calculate the charge deposit at a local position, shift by half matrix in x and y:
        auto deposit_position = ROOT::Math::XYZPoint(
            px + detector->getModel()->getSensorSize().x() / 2, py + detector->getModel()->getSensorSize().y() / 2, pz);

        if(!detector->isWithinSensor(deposit_position)) {
            LOG(WARNING) << "Found deposition outside sensor at " << Units::display(deposit_position, {"mm", "um"})
                         << ". Skipping.";
            continue;
        } else {
            LOG(DEBUG) << "Found deposition inside sensor at " << Units::display(deposit_position, {"mm", "um"})
                       << " in volume " << volume;
        }

        // Calculate number of electron hole pairs produced, taking into acocunt fluctuations between ionization and lattice
        // excitations via the Fano factor. We assume Gaussian statistics here.
        auto mean_charge = static_cast<unsigned int>(Units::get(edep, "keV") / charge_creation_energy_);
        std::normal_distribution<double> charge_fluctuation(mean_charge, std::sqrt(mean_charge * fano_factor_));
        auto charge = charge_fluctuation(random_generator_);

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
        particles_to_deposits[detector].push_back(mc_particles.size() - 1);

        // Deposit hole
        deposits[detector].emplace_back(
            deposit_position, global_deposit_position, CarrierType::HOLE, charge, Units::get(time, "ns"));
        particles_to_deposits[detector].push_back(mc_particles.size() - 1);
    }

    LOG(INFO) << "Finished reading event " << event;

    // Loop over all known detectors and dispatch messages for them
    for(const auto& detector : geo_manager_->getDetectors()) {
        LOG(DEBUG) << detector->getName() << " has " << mc_particles[detector].size() << " MC particles";
        // Send the mc particle information
        auto mc_particle_message = std::make_shared<MCParticleMessage>(std::move(mc_particles[detector]), detector);
        messenger_->dispatchMessage(this, mc_particle_message);

        if(!deposits[detector].empty()) {
            // Assign MCParticles:
            for(size_t i = 0; i < deposits[detector].size(); ++i) {
                deposits[detector].at(i).setMCParticle(
                    &mc_particle_message->getData().at(particles_to_deposits[detector].at(i)));
            }

            LOG(DEBUG) << detector->getName() << " has " << deposits[detector].size() << " deposits";
            // Create a new charge deposit message
            auto deposit_message = std::make_shared<DepositedChargeMessage>(std::move(deposits[detector]), detector);

            // Dispatch the message
            messenger_->dispatchMessage(this, deposit_message);
        }
    }
}
