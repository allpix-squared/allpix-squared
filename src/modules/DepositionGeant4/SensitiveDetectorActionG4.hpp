/**
 * @file
 * @brief Defines the handling of the sensitive device
 * @copyright Copyright (c) 2017-2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_SIMPLE_DEPOSITION_MODULE_SENSITIVE_DETECTOR_ACTION_H
#define ALLPIX_SIMPLE_DEPOSITION_MODULE_SENSITIVE_DETECTOR_ACTION_H

#include <memory>

#include <G4VSensitiveDetector.hh>
#include <G4WrapperProcess.hh>

#include "core/geometry/Detector.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

#include "objects/DepositedCharge.hpp"
#include "objects/MCParticle.hpp"

#include "TrackInfoManager.hpp"

namespace allpix {
    /**
     * @brief Handles the steps of the particles in all sensitive devices
     */
    class SensitiveDetectorActionG4 : public G4VSensitiveDetector {
    public:
        /**
         * @brief Constructs the action handling for every sensitive detector
         * @param detector Detector this sensitive device is bound to
         * @param msg Pointer to the messenger to send the charge deposits
         * @param charge_creation_energy Energy needed per deposited charge
         * @param fano_factor Fano factor for fluctuations in the energy fraction going into e/h pair creation
         * @param random_seed Seed for the random number generator for Fano fluctuations
         */
        SensitiveDetectorActionG4(const std::shared_ptr<Detector>& detector,
                                  TrackInfoManager* track_info_manager,
                                  double charge_creation_energy,
                                  double fano_factor);

        /**
         * @brief Get total number of charges deposited in the sensitive device bound to this action
         */
        unsigned int getTotalDepositedCharge();

        /**
         * @brief Get the number of charges deposited in the sensitive device for this event only.
         * @warning The correct number is only available after dispatching the message, before it refers to the previous
         * event.
         */
        unsigned int getDepositedCharge();

        /**
         * @brief Get the name of the sensitive device bound to this action
         */
        std::string getName();

        /**
         * @brief Set the seed of the associated random number generator
         */
        void seed(uint64_t random_seed) { random_generator_.seed(random_seed); }

        /**
         * @brief Process a single step of a particle passage through this sensor
         * @param step Information about the step
         * @param history Parameter not used
         */
        G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;

        /**
         * @brief Send the MCParticle and DepositedCharge messages
         * @param event_num Event from which the messages are dispatched from
         */
        void dispatchMessages(Event* event);

    private:
        std::shared_ptr<Detector> detector_;
        // Pointer to track info manager to register tracks which pass through sensitive detectors
        TrackInfoManager* track_info_manager_;

        double charge_creation_energy_;
        double fano_factor_;

        // Random number generator for e/h pair creation fluctuation
        std::mt19937_64 random_generator_;

        // Statistics of total and per-event deposited charge
        unsigned int total_deposited_charge_{};
        unsigned int deposited_charge_{};

        // Set of deposited charges in this event
        std::vector<DepositedCharge> deposits_;

        // List of begin points for tracks
        std::map<int, ROOT::Math::XYZPoint> track_begin_;
        // List of end points for tracks
        std::map<int, ROOT::Math::XYZPoint> track_end_;
        // Parent of all mc tracks
        std::map<int, int> track_parents_;
        // PDG code of the tracks
        std::map<int, int> track_pdg_;
        // Arrival timestamp of the tracks
        std::map<int, double> track_time_;

        // Map from deposit index to track id
        std::vector<int> deposit_to_id_;
        // Map from track id to mc particle index
        std::map<int, size_t> id_to_particle_;
    };
} // namespace allpix

#endif /* ALLPIX_SIMPLE_DEPOSITION_MODULE_SENSITIVE_DETECTOR_ACTION_H */
