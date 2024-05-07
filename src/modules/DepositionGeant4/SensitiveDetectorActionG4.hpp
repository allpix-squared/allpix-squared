/**
 * @file
 * @brief Defines the handling of the sensitive device
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_SIMPLE_DEPOSITION_MODULE_SENSITIVE_DETECTOR_ACTION_H
#define ALLPIX_SIMPLE_DEPOSITION_MODULE_SENSITIVE_DETECTOR_ACTION_H

#include <memory>

#include <G4VSensitiveDetector.hh>
#include <G4WrapperProcess.hh>

#include "core/geometry/Detector.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

#include "objects/DepositedCharge.hpp"
#include "objects/MCParticle.hpp"

#include "TrackInfoManager.hpp"

namespace allpix {
    class Event;

    /**
     * @brief Handles the steps of the particles in all sensitive devices
     */
    class SensitiveDetectorActionG4 : public G4VSensitiveDetector {
    public:
        /**
         * @brief Constructs the action handling for every sensitive detector
         * @param detector Detector this sensitive device is bound to
         * @param track_info_manager Pointer to the track information manager
         * @param charge_creation_energy Energy needed per deposited charge
         * @param fano_factor Fano factor for fluctuations in the energy fraction going into e/h pair creation
         * @param cutoff_time Cut-off time for the creation of secondary particles
         */
        SensitiveDetectorActionG4(const std::shared_ptr<Detector>& detector,
                                  TrackInfoManager* track_info_manager,
                                  double charge_creation_energy,
                                  double fano_factor,
                                  double cutoff_time);

        /**
         * @brief Get total number of charges deposited in the sensitive device bound to this action
         */
        unsigned int getTotalDepositedCharge() const;

        /**
         * @brief Get the number of charges deposited in the sensitive device for this event only.
         * @warning The correct number is only available after dispatching the message, before it refers to the previous
         * event.
         */
        unsigned int getDepositedCharge() const;

        /**
         * @brief Get total energy deposited in the sensitive device bound to this action
         */
        double getTotalDepositedEnergy() const;

        /**
         * @brief Get the energy deposited in the sensitive device for this event only.
         * @warning The correct number is only available after dispatching the message, before it refers to the previous
         * event.
         */
        double getDepositedEnergy() const;

        /**
         * @brief Get the position of the incident particle tracks for this event.
         * @warning The current track positions are only available after dispatching the message, before the function
         * returns the track positions of the previous event.
         */
        std::vector<ROOT::Math::XYZPoint> getTrackIncidentPositions() const;

        /**
         * @brief Clears deposition information vectors in preparation for the next event.
         */
        void clearEventInfo();

        /**
         * @brief Get the name of the sensitive device bound to this action
         */
        std::string getName() const;

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
         * @param module The module which is responsible for dispatching the message
         * @param messenger The messenger used to dispatch it
         * @param event Event to dispatch the messages to
         */
        void dispatchMessages(Module* module, Messenger* messenger, Event* event);

    private:
        std::shared_ptr<Detector> detector_;
        // Pointer to track info manager to register tracks which pass through sensitive detectors
        TrackInfoManager* track_info_manager_;

        double charge_creation_energy_;
        double fano_factor_;
        double cutoff_time_;

        /**
         * Random number generator for e/h pair creation fluctuation
         * @note It is okay to keep a separate random number generator here because instances of this class are thread_local
         * and the PRNG is re-seeded every event from the event PRNG. See \ref DepositionGeant4Module::run()
         */
        RandomNumberGenerator random_generator_;

        // Statistics of total and per-event deposited charge
        unsigned int total_deposited_charge_{};
        unsigned int deposited_charge_{};
        double total_deposited_energy_{};
        double deposited_energy_{};

        // List of positions for deposits
        std::vector<ROOT::Math::XYZPoint> deposit_position_;
        std::vector<ROOT::Math::XYZPoint> incident_track_position_;

        std::vector<unsigned int> deposit_charge_;
        std::vector<double> deposit_energy_;
        std::vector<double> deposit_time_;

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
        // Total charge by track
        std::map<int, unsigned int> track_charge_;
        // Total energy by track at start point
        std::map<int, double> track_total_energy_start_;
        // Kinetic energy by track at start point
        std::map<int, double> track_kinetic_energy_start_;

        // Map from deposit index to track id
        std::vector<int> deposit_to_id_;
        // Map from track id to mc particle index
        std::map<int, size_t> id_to_particle_;
    };
} // namespace allpix

#endif /* ALLPIX_SIMPLE_DEPOSITION_MODULE_SENSITIVE_DETECTOR_ACTION_H */
