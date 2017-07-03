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

    // Save entry point for all first steps in volume
    if(step->IsFirstStepInVolume()) {
        track_parents_[step->GetTrack()->GetTrackID()] = step->GetTrack()->GetParentID();

        // Search for the entry at the start of the sensor
        auto track_id = step->GetTrack()->GetTrackID();
        auto entry_position = detector_->getLocalPosition(static_cast<ROOT::Math::XYZPoint>(preStepPoint->GetPosition()));
        while(track_parents_[track_id] != 0 &&
              std::fabs(entry_position.z() - (detector_->getModel()->getSensorCenter().z() -
                                              detector_->getModel()->getSensorSize().z() / 2.0)) > 1e-9) {
            track_id = track_parents_[track_id];
            entry_position = entry_points_[track_id];
        }
        entry_points_[step->GetTrack()->GetTrackID()] = entry_position;
    }
    // Add MCParticle for the last step in the volume if it is at the edge of the sensor
    // FIXME Current method does not make sense if the incoming particle is not the same as the outgoing particle
    if(step->IsLastStepInVolume() &&
       std::fabs(detector_->getLocalPosition(static_cast<ROOT::Math::XYZPoint>(postStepPoint->GetPosition())).z() -
                 (detector_->getModel()->getSensorCenter().z() + detector_->getModel()->getSensorSize().z() / 2.0)) < 1e-9) {
        // Add new MC particle track
        mc_particles_.emplace_back(
            entry_points_[step->GetTrack()->GetTrackID()],
            detector_->getLocalPosition(static_cast<ROOT::Math::XYZPoint>(postStepPoint->GetPosition())),
            step->GetTrack()->GetDynamicParticle()->GetPDGcode());
        id_to_particle_[step->GetTrack()->GetTrackID()] = static_cast<unsigned int>(mc_particles_.size() - 1);
    }

    // Add new deposit if the charge is more than zero
    if(charge == 0) {
        return false;
    }

    deposits_.emplace_back(deposit_position, charge, mid_time);
    deposit_ids_.emplace_back(step->GetTrack()->GetTrackID());

    LOG(DEBUG) << "Created deposit of " << charge << " charges at " << display_vector(mid_pos, {"mm", "um"})
               << " locally on " << display_vector(deposit_position, {"mm", "um"}) << " in " << detector_->getName()
               << " after " << Units::display(mid_time, {"ns", "ps"});

    return true;
}

unsigned int SensitiveDetectorActionG4::getTotalDepositedCharge() {
    return total_deposited_charge_;
}

void SensitiveDetectorActionG4::EndOfEvent(G4HCofThisEvent*) {
    // Always send the track information
    auto mc_particle_message = std::make_shared<MCParticleMessage>(std::move(mc_particles_), detector_);
    messenger_->dispatchMessage(module_, mc_particle_message);

    // Create new mc particle vector
    mc_particles_ = std::vector<MCParticle>();
    id_to_particle_.clear();

    // Send a new message if we have any deposits
    if(!deposits_.empty()) {
        IFLOG(INFO) {
            unsigned int charges = 0;
            for(auto& ch : deposits_) {
                charges += ch.getCharge();
                total_deposited_charge_ += ch.getCharge();
            }
            LOG(INFO) << "Deposited " << charges << " charges in sensor of detector " << detector_->getName();
        }

        // Match deposit with mc particle if possible
        for(size_t i = 0; i < deposits_.size(); ++i) {
            auto iter = id_to_particle_.find(deposit_ids_.at(i));
            if(iter != id_to_particle_.end()) {
                deposits_.at(i).setMCParticle(&mc_particle_message->getData().at(iter->second));
            }
        }

        // Create a new charge deposit message
        auto deposit_message = std::make_shared<DepositedChargeMessage>(std::move(deposits_), detector_);

        // Dispatch the message
        messenger_->dispatchMessage(module_, deposit_message);

        // Make a new empty vector of deposits
        deposits_ = std::vector<DepositedCharge>();
        deposit_ids_.clear();
    }

    // Clear track parents and entry point list
    track_parents_.clear();
    entry_points_.clear();
}
