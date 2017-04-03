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
SensitiveDetectorActionG4::SensitiveDetectorActionG4(std::shared_ptr<Detector> detector,
                                                     Messenger* msg,
                                                     double charge_creation_energy)
    : G4VSensitiveDetector("SensitiveDetector_" + detector->getName()), charge_creation_energy_(charge_creation_energy),
      deposits_(), detector_(detector), messenger_(msg) {}
SensitiveDetectorActionG4::~SensitiveDetectorActionG4() = default;

// process a Geant4 hit interaction
G4bool SensitiveDetectorActionG4::ProcessHits(G4Step* step, G4TouchableHistory*) {
    // get the step parameters
    G4double edep = step->GetTotalEnergyDeposit();
    if(edep < 1e-8) {
        return false;
    }
    G4StepPoint* preStepPoint = step->GetPreStepPoint();
    G4StepPoint* postStepPoint = step->GetPostStepPoint();

    // create a new charge deposit to add to the message
    G4ThreeVector mid_pos = (preStepPoint->GetPosition() + postStepPoint->GetPosition()) / 2;
    // deposit at a position with a certain charge
    ChargeDeposit deposit(allpix::toROOTVector(mid_pos), static_cast<unsigned int>(edep / charge_creation_energy_));
    deposits_.push_back(deposit);

    // LOG(DEBUG) << "energy deposit of " << edep << " between point " << preStepPoint->GetPosition() / um << " and "
    //           << postStepPoint->GetPosition() / um << " in detector " << detector_->getName();

    return true;
}

// send a message at the end of the event
void SensitiveDetectorActionG4::EndOfEvent(G4HCofThisEvent*) {
    // send a new message if we have any deposits
    if(!deposits_.empty()) {
        // create a new charge deposit message
        ChargeDepositMessage deposit_message(std::move(deposits_), detector_);

        // dispatch the message
        messenger_->dispatchMessage(deposit_message, "sensor");

        // make a new empty vector of deposits
        deposits_ = std::vector<ChargeDeposit>();
    }
}
