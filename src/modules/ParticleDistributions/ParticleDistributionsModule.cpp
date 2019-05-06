/**
 * @file
 * @brief Implementation of [ParticleDistributions] module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "ParticleDistributionsModule.hpp"

#include <string>
#include <utility>

#include "core/utils/log.h"

using namespace allpix;

ParticleDistributionsModule::ParticleDistributionsModule(Configuration& config,
                                                         Messenger* messenger,
                                                         GeometryManager* geo_manager)
    : Module(config), messenger_(messenger), geo_manager_(geo_manager) {

    // ... Implement ... (Typically bounds the required messages and optionally sets configuration defaults)
    // Input required by this module
    messenger->bindSingle(this, &ParticleDistributionsModule::message_, MsgFlags::REQUIRED);
}

void ParticleDistributionsModule::init() {

    std::vector<std::shared_ptr<Detector>> detectors = geo_manager_->getDetectors();
    for(auto& detector : detectors) {
        // Get the detector name
        std::string detectorName = detector->getName();
        LOG(DEBUG) << "Detector with name " << detectorName;
    }
    energy_distribution_ = new TH1F("energy_distribution", "energy_distribution", 1000, 0, 15);
    zx_distribution_ = new TH2F("zx_distribution", "zx_distribution", 100, -1, 1, 100, -1, 1);
    zy_distribution_ = new TH2F("zy_distribution", "zy_distribution", 100, -1, 1, 100, -1, 1);
    xyz_distribution_ = new TH3F("xyz_distribution", "xyz_distribution", 100, -1, 1, 100, -1, 1, 100, -1, 1);
    xyz_energy_distribution_ =
        new TH3F("xyz_energy_distribution", "xyz_energy_distribution", 100, -12., 12, 100, -12, 12, 100, -12, 12);

    config_.setDefault<bool>("store_particles", false);
    store_particles_ = config_.get<bool>("store_particles");

    simple_tree_ = new TTree("neutrons", "neutrons");
    simple_tree_->Branch("energy", &energy_);
    simple_tree_->Branch("particle_id", &particle_id_);
    simple_tree_->Branch("start_position_x", &start_position_x_);
    simple_tree_->Branch("start_position_y", &start_position_y_);
    simple_tree_->Branch("start_position_z", &start_position_z_);
    simple_tree_->Branch("momentum_x", &momentum_x_);
    simple_tree_->Branch("momentum_y", &momentum_y_);
    simple_tree_->Branch("momentum_z", &momentum_z_);
}

void ParticleDistributionsModule::run(unsigned int) {

    // Make a store for desired MC tracks
    std::vector<MCTrack> saved_tracks;
    for(auto& particle : message_->getData()) {
        if(particle.getParticleID() != 2112) {
            continue;
        }

        ROOT::Math::XYZVector momentum = particle.getMomentum();
        double magnitude = sqrt(momentum.Mag2());
        double energy = particle.getKineticEnergyInitial();

        ROOT::Math::XYZVector directionVector, energyWeightedDirection;
        directionVector.SetX(momentum.X() / magnitude);
        directionVector.SetY(momentum.Y() / magnitude);
        directionVector.SetZ(momentum.Z() / magnitude);

        energyWeightedDirection.SetX(energy * momentum.X() / magnitude);
        energyWeightedDirection.SetY(energy * momentum.Y() / magnitude);
        energyWeightedDirection.SetZ(energy * momentum.Z() / magnitude);

        energy_distribution_->Fill(energy);
        zx_distribution_->Fill(directionVector.Z(), directionVector.X());
        zy_distribution_->Fill(directionVector.Z(), directionVector.Y());
        xyz_distribution_->Fill(directionVector.X(), directionVector.Y(), directionVector.Z());
        xyz_energy_distribution_->Fill(
            energyWeightedDirection.X(), energyWeightedDirection.Y(), energyWeightedDirection.Z());

        energy_ = particle.getKineticEnergyInitial();
        particle_id_ = particle.getParticleID();
        start_position_x_ = particle.getStartPoint().X();
        start_position_y_ = particle.getStartPoint().Y();
        start_position_z_ = particle.getStartPoint().Z();
        momentum_x_ = momentum.X();
        momentum_y_ = momentum.Y();
        momentum_z_ = momentum.Z();
        simple_tree_->Fill();

        if(store_particles_) {
            saved_tracks.push_back(particle);
        }
    }

    // Dispatch message of pixel charges
    if(store_particles_) {
        auto mcparticle_message = std::make_shared<MCTrackMessage>(saved_tracks);
        messenger_->dispatchMessage(this, mcparticle_message);
    }
}

void ParticleDistributionsModule::finalize() {
    energy_distribution_->Write();
    zx_distribution_->Write();
    zy_distribution_->Write();
    xyz_distribution_->Write();
    xyz_energy_distribution_->Write();
    simple_tree_->Write();
}
