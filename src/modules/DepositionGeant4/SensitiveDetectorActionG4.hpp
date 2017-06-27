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
#include "objects/MCParticle.hpp"

class G4Step;
class G4HCofThisEvent;

namespace allpix {
    class SensitiveDetectorActionG4 : public G4VSensitiveDetector {
    public:
        // Constructor and destructor
        SensitiveDetectorActionG4(Module* module,
                                  const std::shared_ptr<Detector>&,
                                  Messenger*,
                                  double charge_creation_energy);
        ~SensitiveDetectorActionG4() override;

        // Get total deposited charge
        unsigned int getTotalDepositedCharge();

        // Disallow copy
        SensitiveDetectorActionG4(const SensitiveDetectorActionG4&) = delete;
        SensitiveDetectorActionG4& operator=(const SensitiveDetectorActionG4&) = delete;

        // Default move
        SensitiveDetectorActionG4(SensitiveDetectorActionG4&&) = default;
        SensitiveDetectorActionG4& operator=(SensitiveDetectorActionG4&&) = default;

        // Initialize events, process the hits and handle end of event
        G4bool ProcessHits(G4Step*, G4TouchableHistory*) override;
        void EndOfEvent(G4HCofThisEvent*) override;

    private:
        // the conversion factor from energy to charge
        double charge_creation_energy_;

        // charge deposited
        unsigned int total_deposited_charge_{};

        // list of deposits in sensitive device
        std::vector<DepositedCharge> deposits_;

        // list of entry points for all track id
        std::map<int, ROOT::Math::XYZPoint> entry_points_;

        // parent of all tracks
        std::map<int, int> track_parents_;

        // list of all MC particles
        std::vector<MCParticle> mc_particles_;

        // Instantatiation of the deposition module
        Module* module_;

        // the linked detector
        std::shared_ptr<Detector> detector_;

        // link to the messenger
        Messenger* messenger_;
    };
} // namespace allpix

#endif /* ALLPIX_SIMPLE_DEPOSITION_MODULE_SENSITIVE_DETECTOR_ACTION_H */
