/**
 *  @author John Idarraga <idarraga@cern.ch>
 *  @author Koen Wolters <koen.wolters@cern.ch>
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

#include "core/geometry/PixelDetectorModel.hpp"
#include "core/utils/log.h"

#include "tools/geant4.h"

using namespace allpix;

// construct and destruct the sensitive detector
SensitiveDetectorActionG4::SensitiveDetectorActionG4(const std::shared_ptr<Detector>& detector,
                                                     Messenger* msg,
                                                     double charge_creation_energy)
    : G4VSensitiveDetector("SensitiveDetector_" + detector->getName()), charge_creation_energy_(charge_creation_energy),
      deposits_(), detector_(detector), messenger_(msg) {
    // add to the sensitive detector manager
    G4SDManager* sd_man_g4 = G4SDManager::GetSDMpointer();
    sd_man_g4->AddNewDetector(this);
}
SensitiveDetectorActionG4::~SensitiveDetectorActionG4() = default;

// process a Geant4 hit interaction
G4bool SensitiveDetectorActionG4::ProcessHits(G4Step* step, G4TouchableHistory*) {
    // get the step parameters
    auto edep = step->GetTotalEnergyDeposit();
    G4StepPoint* preStepPoint = step->GetPreStepPoint();
    G4StepPoint* postStepPoint = step->GetPostStepPoint();

    // put the charge deposit in the middle of the step
    G4ThreeVector mid_pos = (preStepPoint->GetPosition() + postStepPoint->GetPosition()) / 2;

    // create the charge deposit at a local position
    auto deposit_position = detector_->getLocalPosition(static_cast<ROOT::Math::XYZPoint>(mid_pos));
    auto charge = static_cast<unsigned int>(edep / charge_creation_energy_);
    if(charge == 0) {
        return false;
    }
    DepositedCharge deposit(deposit_position, charge);
    deposits_.push_back(deposit);

    LOG(DEBUG) << "created deposit of " << charge << " charges at " << mid_pos << " locally on " << deposit_position;

    return true;
}

// send a message at the end of the event
void SensitiveDetectorActionG4::EndOfEvent(G4HCofThisEvent*) {
    // send a new message if we have any deposits
    if(!deposits_.empty()) {
        IFLOG(INFO) {
            uint32_t charges = 0;
            for(auto ch : deposits_) {
                charges += ch.getCharge();
            }
            LOG(INFO) << "Deposited " << charges << " charge carriers in sensor of detector \"" << detector_->getName()
                      << "\"";
        }

        // create a new charge deposit message
        DepositedChargeMessage deposit_message(std::move(deposits_), detector_);

        // dispatch the message
        messenger_->dispatchMessage(deposit_message, "sensor");

        // make a new empty vector of deposits
        deposits_ = std::vector<DepositedCharge>();
    }
}
