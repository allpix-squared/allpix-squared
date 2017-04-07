/**
 *  @author John Idarraga <idarraga@cern.ch>
 */

#ifndef ALLPIX_SIMPLE_DEPOSITION_MODULE_SENSITIVE_DETECTOR_ACTION_H
#define ALLPIX_SIMPLE_DEPOSITION_MODULE_SENSITIVE_DETECTOR_ACTION_H

#include <memory>

#include <G4VSensitiveDetector.hh>
#include <G4WrapperProcess.hh>

#include "core/geometry/Detector.hpp"
#include "core/messenger/Messenger.hpp"

#include "objects/DepositedCharge.hpp"

class G4Step;
class G4HCofThisEvent;
class G4Event;

namespace allpix {
    class PixelDetectorModel;

    class SensitiveDetectorActionG4 : public G4VSensitiveDetector {
    public:
        // Constructor and destructor
        SensitiveDetectorActionG4(std::shared_ptr<Detector>, Messenger*, double charge_creation_energy);
        ~SensitiveDetectorActionG4() override;

        // Disallow copy
        SensitiveDetectorActionG4(const SensitiveDetectorActionG4&) = delete;
        SensitiveDetectorActionG4& operator=(const SensitiveDetectorActionG4&) = delete;

        // Initialize events, process the hits and handle end of event
        G4bool ProcessHits(G4Step*, G4TouchableHistory*) override;
        void EndOfEvent(G4HCofThisEvent*) override;

    private:
        // the conversion factor from energy to charge
        double charge_creation_energy_;

        // list of deposits in sensitive device
        std::vector<DepositedCharge> deposits_;

        // the linked detector
        std::shared_ptr<Detector> detector_;

        // link to the messenger
        Messenger* messenger_;
    };
} // namespace allpix

#endif /* ALLPIX_SIMPLE_DEPOSITION_MODULE_SENSITIVE_DETECTOR_ACTION_H */
