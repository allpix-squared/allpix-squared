/**
 *  @author John Idarraga <idarraga@cern.ch>
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "SensitiveDetectorG4.hpp"

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

// FIXME: should get rid of CLHEP units
#include <CLHEP/Units/SystemOfUnits.h>

using namespace CLHEP;

using namespace allpix;

// temp
G4double g_temp_edep = 0.;
G4int g_temp_pdgId = 0;

// construct and destruct the sensitive detector
SensitiveDetectorG4::SensitiveDetectorG4(std::shared_ptr<Detector> detector, Messenger* msg)
    : G4VSensitiveDetector("SensitiveDetector_" + detector->getName()), deposits_(), detector_(detector), messenger_(msg),
      m_firstStrikePrimary(false), m_kinEPrimary(0), m_totalEdep(0) {}
SensitiveDetectorG4::~SensitiveDetectorG4() = default;

// process a Geant4 hit interaction
G4bool SensitiveDetectorG4::ProcessHits(G4Step* step, G4TouchableHistory*) {
    // track
    G4Track* aTrack = step->GetTrack();
    // particle
    // G4ParticleDefinition *aParticle = aTrack->GetDefinition();

    // check if this is the first interaction in the detector
    if(aTrack->GetTrackID() == 1 && !m_firstStrikePrimary) {
        m_kinEPrimary = aTrack->GetKineticEnergy() / keV;
        m_kinEPrimary -= (step->GetTotalEnergyDeposit() / keV);
        m_firstStrikePrimary = true;
    }

    // crate the hit
    G4double edep = step->GetTotalEnergyDeposit();
    if(edep == 0.) {
        return false;
    }

    G4StepPoint* preStepPoint = step->GetPreStepPoint();
    G4StepPoint* postStepPoint = step->GetPostStepPoint();

    // create a new charge deposit to add to the message
    G4ThreeVector mid_pos = (preStepPoint->GetPosition() + postStepPoint->GetPosition()) / 2;
    ChargeDeposit deposit(allpix::toROOTVector(mid_pos), edep);
    deposits_.push_back(deposit);

    LOG(DEBUG) << "energy deposit of " << edep << " between point " << preStepPoint->GetPosition() / um << " and "
               << postStepPoint->GetPosition() / um << " in detector " << detector_->getName();

    if(m_totalEdep > m_kinEPrimary) {
        LOG(WARNING) << "total energy deposit more than kinetic energy (m_totalEdep = " << m_totalEdep
                     << ", m_kinEPrimary = " << m_kinEPrimary << ")";
    }

    return true;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void SensitiveDetectorG4::EndOfEvent(G4HCofThisEvent*) {
    // send a new message if we have any deposits
    if(!deposits_.empty()) {
        // create a new charge deposit message
        ChargeDepositMessage deposit_message(std::move(deposits_), detector_);

        // dispatch the message
        messenger_->dispatchMessage(deposit_message);

        // make a new empty vector of deposits
        deposits_ = std::vector<ChargeDeposit>();
    }

    // clear the Set of pointers to hitCollection used for verification
    m_firstStrikePrimary = false;
    m_totalEdep = 0.;
    m_kinEPrimary = 0.;
}
