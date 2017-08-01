/**
 * @file
 * @brief Defines the handling of the sensitive device
 * @copyright MIT License
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

namespace allpix {
    /**
     * @brief Handles the steps of the particles in all sensitive devices
     */
    class SensitiveDetectorActionG4 : public G4VSensitiveDetector {
    public:
        /**
         * @brief Constructs the action handling for every sensitive detector
         * @param module Pointer to the DepositionGeant4 module holding this class
         * @param detector Detector this sensitive device is bound to
         * @param msg Pointer to the messenger to send the charge deposits
         * @param charge_creation_energy Energy needed per deposited charge
         */
        SensitiveDetectorActionG4(Module* module,
                                  const std::shared_ptr<Detector>& detector,
                                  Messenger* msg,
                                  double charge_creation_energy);

        /**
         * @brief Get total of charges deposited in the sensitive device bound to this action
         */
        unsigned int getTotalDepositedCharge();

        /**
         * @brief Process a single step of a particle passage through this sensor
         * @param step Information about the step
         * @param history Parameter not used
         */
        G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;

        /**
             * @brief Send the DepositedCharge Message
             */
        void dispatchDepositedChargeMessage();

    private:
        // Instantatiation of the deposition module
        Module* module_;
        std::shared_ptr<Detector> detector_;
        Messenger* messenger_;

        double charge_creation_energy_;

        // Statistics of total deposited charge
        unsigned int total_deposited_charge_{};

        // Set of deposited charges in this event
        std::vector<DepositedCharge> deposits_;
        // List of ids for every deposit
        std::vector<int> deposit_ids_;

        // List of entry points for all tracks
        std::map<int, ROOT::Math::XYZPoint> entry_points_;
        // Parent of all tracks
        std::map<int, int> track_parents_;

        // List of all MC particles
        std::vector<MCParticle> mc_particles_;
        // Conversions from id to particle index
        std::map<int, unsigned int> id_to_particle_;
    };
} // namespace allpix

#endif /* ALLPIX_SIMPLE_DEPOSITION_MODULE_SENSITIVE_DETECTOR_ACTION_H */
