/**
 * @file
 * @brief Implements the handling of the sensitive device
 * @remarks Based on code from John Idarraga
 * @copyright MIT License
 */

#include "SensitiveDetectorActionG4.hpp"

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

#include "core/utils/log.h"
#include "tools/ROOT.h"
#include "tools/geant4.h"

using namespace allpix;

SensitiveDetectorActionG4::SensitiveDetectorActionG4(Module* module,
                                                     const std::shared_ptr<Detector>& detector,
                                                     Messenger* msg,
                                                     double charge_creation_energy)
    : G4VSensitiveDetector("SensitiveDetector_" + detector->getName()), module_(module), detector_(detector),
      messenger_(msg), charge_creation_energy_(charge_creation_energy) {

    // Add the sensor to the internal sensitive detector manager
    G4SDManager* sd_man_g4 = G4SDManager::GetSDMpointer();
    sd_man_g4->AddNewDetector(this);
}

G4bool SensitiveDetectorActionG4::ProcessHits(G4Step* step, G4TouchableHistory*) {
    // Get the step parameters
    auto edep = step->GetTotalEnergyDeposit();
    G4StepPoint* preStepPoint = step->GetPreStepPoint();
    G4StepPoint* postStepPoint = step->GetPostStepPoint();

    // Put the charge deposit in the middle of the step
    G4ThreeVector mid_pos = (preStepPoint->GetPosition() + postStepPoint->GetPosition()) / 2;
    double mid_time = (preStepPoint->GetGlobalTime() + postStepPoint->GetGlobalTime()) / 2;

    // Calculate the charge deposit at a local position
    auto deposit_position = detector_->getLocalPosition(static_cast<ROOT::Math::XYZPoint>(mid_pos));
    auto charge = static_cast<unsigned int>(edep / charge_creation_energy_);

    // Save begin point when track is seen for the first time
    if(track_begin_.find(step->GetTrack()->GetTrackID()) == track_begin_.end()) {
        auto start_position = detector_->getLocalPosition(static_cast<ROOT::Math::XYZPoint>(preStepPoint->GetPosition()));
        track_begin_.emplace(step->GetTrack()->GetTrackID(), start_position);

        track_parents_.emplace(step->GetTrack()->GetTrackID(), step->GetTrack()->GetParentID());
        track_pdg_.emplace(step->GetTrack()->GetTrackID(), step->GetTrack()->GetDynamicParticle()->GetPDGcode());
    }

    // Update current end point with the current last step
    auto end_position = detector_->getLocalPosition(static_cast<ROOT::Math::XYZPoint>(postStepPoint->GetPosition()));
    track_end_[step->GetTrack()->GetTrackID()] = end_position;

    // Add new deposit if the charge is more than zero
    if(charge == 0) {
        return false;
    }

    auto global_deposit_position = detector_->getGlobalPosition(deposit_position);

    // Deposit electron
    deposits_.emplace_back(deposit_position, global_deposit_position, CarrierType::ELECTRON, charge, mid_time);
    deposit_to_id_.push_back(step->GetTrack()->GetTrackID());

    // Deposit hole
    deposits_.emplace_back(deposit_position, global_deposit_position, CarrierType::HOLE, charge, mid_time);
    deposit_to_id_.push_back(step->GetTrack()->GetTrackID());

    LOG(DEBUG) << "Created deposit of " << charge << " charges at " << display_vector(mid_pos, {"mm", "um"})
               << " locally on " << display_vector(deposit_position, {"mm", "um"}) << " in " << detector_->getName()
               << " after " << Units::display(mid_time, {"ns", "ps"});

    return true;
}

unsigned int SensitiveDetectorActionG4::getTotalDepositedCharge() {
    return total_deposited_charge_;
}

void SensitiveDetectorActionG4::dispatchMessages() {
    // Create the mc particles
    std::vector<MCParticle> mc_particles;
    for(auto& track_id_point : track_begin_) {

        auto track_id = track_id_point.first;
        auto local_begin = track_id_point.second;

        ROOT::Math::XYZPoint end_point;
        auto local_end = track_end_.at(track_id);
        auto pdg_code = track_pdg_.at(track_id);

        auto global_begin = detector_->getGlobalPosition(local_begin);
        auto global_end = detector_->getGlobalPosition(local_end);
        mc_particles.emplace_back(local_begin, global_begin, local_end, global_end, pdg_code);
        id_to_particle_[track_id] = mc_particles.size() - 1;
    }

    // Send the mc particle information
    auto mc_particle_message = std::make_shared<MCParticleMessage>(std::move(mc_particles), detector_);
    messenger_->dispatchMessage(module_, mc_particle_message);

    // Clear track data for the next event
    track_parents_.clear();
    track_begin_.clear();
    track_end_.clear();
    track_pdg_.clear();

    // Send a deposit message if we have any deposits
    if(!deposits_.empty()) {
        unsigned int charges = 0;
        for(auto& ch : deposits_) {
            charges += ch.getCharge();
            total_deposited_charge_ += ch.getCharge();
        }
        LOG(INFO) << "Deposited " << charges << " charges in sensor of detector " << detector_->getName();

        // Match deposit with mc particle if possible
        for(size_t i = 0; i < deposits_.size(); ++i) {
            auto track_id = deposit_to_id_.at(i);
            deposits_.at(i).setMCParticle(&mc_particle_message->getData().at(id_to_particle_.at(track_id)));
        }

        // Create a new charge deposit message
        auto deposit_message = std::make_shared<DepositedChargeMessage>(std::move(deposits_), detector_);

        // Dispatch the message
        messenger_->dispatchMessage(module_, deposit_message);
    }

    // Clear deposits for next event
    deposits_ = std::vector<DepositedCharge>();

    // Clear link tables for next event
    deposit_to_id_.clear();
    id_to_particle_.clear();
}
