/**
 * @file
 * @brief Implements the handling of the sensitive device
 * @remarks Based on code from John Idarraga
 *
 * @copyright Copyright (c) 2017-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "SensitiveDetectorActionG4.hpp"
#include "TrackInfoG4.hpp"

#include <memory>

#include "G4DecayTable.hh"
#include "G4HCofThisEvent.hh"
#include "G4LogicalVolume.hh"
#include "G4RunManager.hh"
#include "G4SDManager.hh"
#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4Track.hh"
#include "G4VProcess.hh"
#include "G4ios.hh"

#include "TMath.h"
#include "TString.h"

#include "core/utils/distributions.h"
#include "core/utils/log.h"
#include "tools/ROOT.h"
#include "tools/geant4/geant4.h"

using namespace allpix;

SensitiveDetectorActionG4::SensitiveDetectorActionG4(const std::shared_ptr<Detector>& detector,
                                                     TrackInfoManager* track_info_manager,
                                                     double charge_creation_energy,
                                                     double fano_factor,
                                                     double cutoff_time)
    : G4VSensitiveDetector("SensitiveDetector_" + detector->getName()), detector_(detector),
      track_info_manager_(track_info_manager), charge_creation_energy_(charge_creation_energy), fano_factor_(fano_factor),
      cutoff_time_(cutoff_time) {

    // Add the sensor to the internal sensitive detector manager
    G4SDManager* sd_man_g4 = G4SDManager::GetSDMpointer();
    sd_man_g4->AddNewDetector(this);
}

G4bool SensitiveDetectorActionG4::ProcessHits(G4Step* step, G4TouchableHistory*) {
    // Get the step parameters
    auto edep = step->GetTotalEnergyDeposit();
    G4StepPoint* preStep = step->GetPreStepPoint();
    G4StepPoint* postStep = step->GetPostStepPoint();
    LOG(TRACE) << "Distance of this step: " << (postStep->GetPosition() - preStep->GetPosition()).mag();

    // Get Transportaion Matrix
    G4TouchableHandle theTouchable = step->GetPreStepPoint()->GetTouchableHandle();
    auto* track = step->GetTrack();

    // Put the charge deposit in the middle of the step unless it is a photon:
    auto is_photon = (track->GetDynamicParticle()->GetPDGcode() == 22);
    LOG(TRACE) << "Placing energy deposit "
               << (is_photon ? "at the end of step, photon detected" : "in the middle of the step");
    G4ThreeVector step_pos = is_photon ? postStep->GetPosition() : (preStep->GetPosition() + postStep->GetPosition()) / 2;
    double step_time = is_photon ? postStep->GetGlobalTime() : (preStep->GetGlobalTime() + postStep->GetGlobalTime()) / 2;

    // If this arrives very late, skip MCParticle and DepositedCharge creation:
    if(step_time > cutoff_time_) {
        return false;
    }

    // Calculate the charge deposit at a local position
    auto deposit_position = detector_->getLocalPosition(static_cast<ROOT::Math::XYZPoint>(step_pos));

    // Calculate number of electron hole pairs produced, taking into account fluctuations between ionization and lattice
    // excitations via the Fano factor. We assume Gaussian statistics here.
    auto mean_charge = edep / charge_creation_energy_;
    allpix::normal_distribution<double> charge_fluctuation(mean_charge, std::sqrt(mean_charge * fano_factor_));
    auto charge = static_cast<unsigned int>(std::max(charge_fluctuation(random_generator_), 0.) + 0.5);

    const auto* userTrackInfo = dynamic_cast<TrackInfoG4*>(track->GetUserInformation());
    if(userTrackInfo == nullptr) {
        throw ModuleError("No track information attached to track.");
    }
    auto trackID = userTrackInfo->getID();

    // If this track originates in the sensor add parent ID. Otherwise set the ID to zero (primary particle) since it might
    // have a parent connected from a previous crossing of the sensor, i.e. backscattering from an interaction in non-sensor
    // material. While these particles are connected via MCTracks, we treat them as primaries to the sensor since they
    // entered from the outside and were not created in the sensor volume.
    auto parentTrackID =
        (track->GetVolume()->GetLogicalVolume() == track->GetLogicalVolumeAtVertex() ? userTrackInfo->getParentID() : 0);

    // Save begin point when track is seen for the first time
    if(track_begin_.find(trackID) == track_begin_.end()) {
        track_info_manager_->setTrackInfoToBeStored(trackID);
        auto start_position = detector_->getLocalPosition(static_cast<ROOT::Math::XYZPoint>(preStep->GetPosition()));
        track_begin_.emplace(trackID, start_position);
        track_parents_.emplace(trackID, parentTrackID);
        track_time_.emplace(trackID, step_time);
        track_pdg_.emplace(trackID, track->GetDynamicParticle()->GetPDGcode());
        track_total_energy_start_.emplace(trackID, track->GetTotalEnergy());
        track_kinetic_energy_start_.emplace(trackID, track->GetKineticEnergy());
    }

    // Update current end point with the current last step
    track_end_[trackID] = detector_->getLocalPosition(static_cast<ROOT::Math::XYZPoint>(postStep->GetPosition()));
    track_charge_[trackID] += charge;

    // Add new deposit if the charge is more than zero
    if(charge == 0) {
        return false;
    }

    // Store relevant quantities to create charge deposits:
    deposit_position_.push_back(deposit_position);
    deposit_charge_.push_back(charge);
    deposit_energy_.push_back(edep);
    deposit_time_.push_back(step_time);
    deposit_to_id_.push_back(trackID);

    return true;
}

std::string SensitiveDetectorActionG4::getName() const { return detector_->getName(); }

unsigned int SensitiveDetectorActionG4::getTotalDepositedCharge() const { return total_deposited_charge_; }

unsigned int SensitiveDetectorActionG4::getDepositedCharge() const { return deposited_charge_; }

double SensitiveDetectorActionG4::getTotalDepositedEnergy() const { return total_deposited_energy_; }

double SensitiveDetectorActionG4::getDepositedEnergy() const { return deposited_energy_; }

void SensitiveDetectorActionG4::clearEventInfo() {
    LOG(DEBUG) << "Clearing track and deposit vectors";

    track_parents_.clear();
    track_begin_.clear();
    track_end_.clear();
    track_pdg_.clear();
    track_time_.clear();
    track_charge_.clear();
    track_total_energy_start_.clear();
    track_kinetic_energy_start_.clear();

    deposit_position_.clear();
    deposit_charge_.clear();
    deposit_energy_.clear();
    deposit_time_.clear();

    deposit_to_id_.clear();
    id_to_particle_.clear();
}

void SensitiveDetectorActionG4::dispatchMessages(Module* module, Messenger* messenger, Event* event) {

    auto time_reference = std::min_element(track_time_.begin(), track_time_.end(), [](const auto& l, const auto& r) {
                              return l.second < r.second;
                          })->second;
    LOG(TRACE) << "Earliest MCParticle arrived at " << Units::display(time_reference, {"ns", "ps"}) << " global";

    // Create the mc particles
    std::vector<MCParticle> mc_particles;
    for(auto& track_id_point : track_begin_) {
        auto track_id = track_id_point.first;
        auto local_begin = track_id_point.second;

        ROOT::Math::XYZPoint end_point;
        auto local_end = track_end_.at(track_id);
        auto pdg_code = track_pdg_.at(track_id);
        auto charge = track_charge_.at(track_id);
        auto track_time_global = track_time_.at(track_id);
        auto track_time_local = track_time_global - time_reference;

        auto global_begin = detector_->getGlobalPosition(local_begin);
        auto global_end = detector_->getGlobalPosition(local_end);
        mc_particles.emplace_back(
            local_begin, global_begin, local_end, global_end, pdg_code, track_time_local, track_time_global);
        // Count electrons and holes:
        mc_particles.back().setTotalDepositedCharge(2 * charge);
        mc_particles.back().setTrack(track_info_manager_->findMCTrack(track_id));
        mc_particles.back().setTotalEnergyStart(track_total_energy_start_.at(track_id));
        mc_particles.back().setKineticEnergyStart(track_kinetic_energy_start_.at(track_id));
        id_to_particle_[track_id] = mc_particles.size() - 1;

        LOG(DEBUG) << "Found MC particle " << pdg_code << " crossing detector " << detector_->getName() << " from "
                   << Units::display(local_begin, {"mm", "um"}) << " to " << Units::display(local_end, {"mm", "um"})
                   << " local after " << Units::display(track_time_global, {"ns", "ps"}) << " global / "
                   << Units::display(track_time_local, {"ns", "ps"}) << " local";
    }

    for(auto& track_parent : track_parents_) {
        auto track_id = track_parent.first;
        auto parent_id = track_parent.second;
        if(id_to_particle_.find(parent_id) == id_to_particle_.end()) {
            // Skip tracks without direct parents with deposits
            // FIXME: Geant4 does not allow for an easy way retrieve the whole hierarchy
            continue;
        }
        auto track_idx = id_to_particle_.at(track_id);
        auto parent_idx = id_to_particle_.at(parent_id);
        mc_particles.at(track_idx).setParent(&mc_particles.at(parent_idx));
    }

    // Send the mc particle information
    auto mc_particle_message = std::make_shared<MCParticleMessage>(std::move(mc_particles), detector_);
    messenger->dispatchMessage(module, mc_particle_message, event);

    // Send a deposit message if we have any deposits
    unsigned int charges = 0;
    double energies = 0.;
    if(!deposit_position_.empty()) {
        // Prepare charge deposits for this event
        std::vector<DepositedCharge> deposits;
        for(size_t i = 0; i < deposit_position_.size(); i++) {
            auto local_position = deposit_position_.at(i);
            auto global_position = detector_->getGlobalPosition(local_position);

            auto global_time = deposit_time_.at(i);
            auto local_time = global_time - time_reference;

            auto charge = deposit_charge_.at(i);
            charges += 2 * charge;
            total_deposited_charge_ += 2 * charge;

            auto edep = deposit_energy_.at(i);
            total_deposited_energy_ += edep;
            energies += edep;

            // Match deposit with mc particle if possible
            auto track_id = deposit_to_id_.at(i);

            // Deposit electron
            deposits.emplace_back(local_position, global_position, CarrierType::ELECTRON, charge, local_time, global_time);
            deposits.back().setMCParticle(&mc_particle_message->getData().at(id_to_particle_.at(track_id)));

            // Deposit hole
            deposits.emplace_back(local_position, global_position, CarrierType::HOLE, charge, local_time, global_time);
            deposits.back().setMCParticle(&mc_particle_message->getData().at(id_to_particle_.at(track_id)));

            LOG(DEBUG) << "Created deposit of " << charge << " charges at " << Units::display(global_position, {"mm", "um"})
                       << " global / " << Units::display(local_position, {"mm", "um"}) << " local in "
                       << detector_->getName() << " after " << Units::display(global_time, {"ns", "ps"}) << " global / "
                       << Units::display(local_time, {"ns", "ps"}) << " local";
        }

        LOG(INFO) << "Deposited " << charges << " charges in sensor of detector " << detector_->getName();

        // Create a new charge deposit message
        auto deposit_message = std::make_shared<DepositedChargeMessage>(std::move(deposits), detector_);

        // Dispatch the message
        messenger->dispatchMessage(module, std::move(deposit_message), event);
    }
    // Store the number of charge carriers:
    deposited_charge_ = charges;
    deposited_energy_ = energies;

    // Clear track data, deposit information, and link tables for next event
    clearEventInfo();
}
