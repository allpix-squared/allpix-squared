/**
 *  Author John Idarraga <idarraga@cern.ch>
 */

#ifndef ALLPIX_SIMPLE_DEPOSITION_MODULE_SENSITIVE_DETECTOR_H
#define ALLPIX_SIMPLE_DEPOSITION_MODULE_SENSITIVE_DETECTOR_H

#include "G4VSensitiveDetector.hh"
#include "G4WrapperProcess.hh"

#include <set>

#include "../../core/geometry/Detector.hpp"
#include "../../core/messenger/Messenger.hpp"

#include "../../messages/DepositionMessage.hpp"

class G4Step;
class G4HCofThisEvent;
class G4Event;

namespace allpix{
    class PixelDetectorModel;
    
    class SensitiveDetectorG4 : public G4VSensitiveDetector {
    public:
        SensitiveDetectorG4(std::shared_ptr<Detector>, Messenger *);
        ~SensitiveDetectorG4();
        
        void Initialize(G4HCofThisEvent*);
        G4bool ProcessHits(G4Step*, G4TouchableHistory*);
        void EndOfEvent(G4HCofThisEvent*);
    private:
        std::shared_ptr<DepositionMessage> deposit_message_;
        
        // the linked detector
        std::shared_ptr<Detector> detector_; 
        
        // link to the messenger
        Messenger *messenger_;
        
        bool m_firstStrikePrimary;
        G4double m_kinEPrimary;
        G4double m_totalEdep;
    };
}

#endif /* ALLPIX_SIMPLE_DEPOSITION_MODULE_SENSITIVE_DETECTOR_H */

