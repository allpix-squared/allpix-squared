/**
 *  @author John Idarraga <idarraga@cern.ch>
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "SensitiveDetectorG4.hpp"

#include "G4HCofThisEvent.hh"
#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4SDManager.hh"
#include "G4RunManager.hh"
#include "G4ios.hh"
#include "G4Track.hh"
#include "G4VProcess.hh"
#include "G4DecayTable.hh"
#include "G4LogicalVolume.hh"

#include "TMath.h"
#include "TString.h"

#include "CLHEP/Units/SystemOfUnits.h"
using namespace CLHEP;

#include "../../core/geometry/PixelDetectorModel.hpp"
#include "../../core/utils/log.h"

using namespace allpix;

// temp
G4double g_temp_edep = 0.;
G4int g_temp_pdgId = 0;

// construct and destruct the sensitive detector
SensitiveDetectorG4::SensitiveDetectorG4(std::shared_ptr<Detector> detector) 
                                        : G4VSensitiveDetector("SensitiveDetector_"+detector->getName()), detector_(detector),
                                          m_firstStrikePrimary(false), m_totalEdep(0) {}
SensitiveDetectorG4::~SensitiveDetectorG4() {}

// run once per event the initialization
void SensitiveDetectorG4::Initialize(G4HCofThisEvent*) {}

// process a Geant4 hit interaction
G4bool SensitiveDetectorG4::ProcessHits(G4Step * step, G4TouchableHistory *)
{    
    // track
    G4Track *aTrack = step->GetTrack();
    // particle
    G4ParticleDefinition *aParticle = aTrack->GetDefinition();
    
    // check if this is the first interaction in the detector
    if ( aTrack->GetTrackID() == 1 && ! m_firstStrikePrimary ) {
        m_kinEPrimary = aTrack->GetKineticEnergy()/keV;
        m_kinEPrimary -= (step->GetTotalEnergyDeposit()/keV);
        m_firstStrikePrimary = true;
    }
    
    // crate the hit
    G4double edep = step->GetTotalEnergyDeposit();
    if(edep==0.) return false;
    
    G4StepPoint * preStepPoint = step->GetPreStepPoint();
    G4StepPoint * postStepPoint = step->GetPostStepPoint();
    
    const G4TouchableHandle touchablepre = preStepPoint->GetTouchableHandle();
    const G4TouchableHandle touchablepost = postStepPoint->GetTouchableHandle();
    
    G4int copyIDy_pre  = -1;
    G4int copyIDx_pre  = -1;
    G4int copyIDy_post = -1;
    G4int copyIDx_post = -1;
    G4ThreeVector correctedPos(0,0,0);
    G4ThreeVector PosOnChip(0,0,0);
            
    /* CONVERT SOME POSITIONS !
        FIXME: do this in the new way...
    
    // This positions are global, I will bring them to pixel-centered frame
    // I can use the physical volumes for that
    G4ThreeVector prePos = preStepPoint->GetPosition();

    // Find the inverse rotation
    //G4RotationMatrix invRot2 = CLHEP::inverseOf(*m_rotationOfWrapper);
    G4RotationMatrix invRot = m_rotationOfWrapper->inverse().inverse();

    // Absolute center of Si wafer
    G4ThreeVector absCenterOfDetector = m_absolutePosOfWrapper ;

    // Bring the detector (Si layer) to the Origin
    correctedPos = prePos;
    correctedPos -= absCenterOfDetector;
    // apply rotation !
    correctedPos = invRot * correctedPos;
    PosOnChip = correctedPos - m_relativePosOfSD;

    // Now let's finally provide pixel-centered coordinates for each hit
    // Build the center of the Pixel
    auto model = std::dynamic_pointer_cast<PixelDetectorModel>(detector_->getModel());
    assert(model); //FIXME: model does not have to be a pixel detector...
    
    G4ThreeVector centerOfPixel(
        model->GetPixelX()*TMath::FloorNint(correctedPos.x() / model->GetPixelX()) + model->GetHalfPixelX(),
                                model->GetPixelY()*TMath::FloorNint(correctedPos.y() / model->GetPixelY()) + model->GetHalfPixelY(),
                                0.); // in the middle of the tower

    // The position within the pixel !!!

    // 20160316: The line below wrong and only works if the relative postion is (0,0,0)   
    // correctedPos = correctedPos - centerOfPixel - m_relativePosOfSD;
    correctedPos = correctedPos - centerOfPixel;


    // Work with the Hit
    //G4cout << "uncorrectedPos : " << prePos.x()/um << " " << prePos.y()/um
    //	   << " " << prePos.z()/um << " [um]" << G4endl;

    //G4cout << "correctedPos : " << correctedPos.x()/um << " " << correctedPos.y()/um
    //	   << " " << correctedPos.z()/um << " [um]" << G4endl;

    //G4cout << "(" << shi << ") "<<	 correctedPos.z()/um << " " ;
    //G4cout << TString::Format("(%02.0f) %02.1f ",shi,correctedPos.z()/um);

    // depth 1 --> x
    // depth 0 --> y
    copyIDy_pre  = touchablepre->GetCopyNumber();
    copyIDx_pre  = touchablepre->GetCopyNumber(1);
    
    // POST //
    
    // Look at the touchablepost only if in the same volume, i.e. in the sensitive Si Box
    // If the hit is in a different pixel, it is still the same phys volume
    // The problem is that if I the postStep is in a different volume, GetCopyNumber(1)
    //  doesn't make sense anymore.
    G4ThreeVector postPos(0,0,0);
    if(preStepPoint->GetPhysicalVolume() == postStepPoint->GetPhysicalVolume()){
        postPos = postStepPoint->GetPosition();
        copyIDy_post = touchablepost->GetCopyNumber();
        copyIDx_post = touchablepost->GetCopyNumber(1);
    }
    */
    
    // process
    const G4VProcess * aProcessPointer = step->GetPostStepPoint()->GetProcessDefinedStep();
    
    // FIXME: save the actual information about the hit hit somewhere
    steps_.push_back(step);
    LOG(DEBUG) << "energy deposit of " << edep << " between point " << preStepPoint->GetPosition()/um << " and " << postStepPoint->GetPosition()/um <<  " in detector " << detector_->getName();
    
    if ( m_totalEdep > m_kinEPrimary ) {
        LOG(WARNING) << "total energy deposit more than kinetic energy (m_totalEdep = " << m_totalEdep << ", m_kinEPrimary = " << m_kinEPrimary << ")";
    }
    
    return true;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void SensitiveDetectorG4::EndOfEvent(G4HCofThisEvent*)
{    
    // clear the Set of pointers to hitCollection used for verification
    m_firstStrikePrimary = false;
    m_totalEdep = 0.;
    m_kinEPrimary = 0.;
    
}
