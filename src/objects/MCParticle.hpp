/**
 * @file
 * @brief Definition of Monte-Carlo particle object
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MC_PARTICLE_H
#define ALLPIX_MC_PARTICLE_H

#include <Math/Point3D.h>
#include <TRef.h>

#include "MCTrack.hpp"
#include "Object.hpp"

namespace allpix {
    /**
     * @ingroup Objects
     * @brief Monte-Carlo particle through the sensor
     */
    class MCParticle : public Object {
    public:
        /**
         * @brief Construct a Monte-Carlo particle
         * @param local_start_point Entry point of the particle in the sensor in local coordinates
         * @param global_start_point Entry point of the particle in the sensor in global coordinates
         * @param local_end_point Exit point of the particle in the sensor in local coordinates
         * @param global_end_point Exit point of the particle in the sensor in global coordinates
         * @param particle_id PDG id for this particle type
         * @param local_time The arrival time of the particle in the sensor in local coordinates
         * @param global_time The arrival time of the particle in the sensor in global coordinates
         */
        MCParticle(ROOT::Math::XYZPoint local_start_point,
                   ROOT::Math::XYZPoint global_start_point,
                   ROOT::Math::XYZPoint local_end_point,
                   ROOT::Math::XYZPoint global_end_point,
                   int particle_id,
                   double local_time,
                   double global_time);

        /**
         * @brief Get the entry point of the particle in local coordinates
         * @return Particle entry point
         */
        ROOT::Math::XYZPoint getLocalStartPoint() const;
        /**
         * @brief Get the entry point of the particle in global coordinates
         * @return Particle entry point
         */
        ROOT::Math::XYZPoint getGlobalStartPoint() const;

        /**
         * @brief Get the exit point of the particle in local coordinates
         * @return Particle exit point
         */
        ROOT::Math::XYZPoint getLocalEndPoint() const;
        /**
         * @brief Get the exit point of the particle in global coordinates
         * @return Particle exit point
         */
        ROOT::Math::XYZPoint getGlobalEndPoint() const;

        /**
         * @brief Get the reference point of the particle in the sensor center plane in local coordinates
         * @return Particle reference point on the center plane of the sensor
         */
        ROOT::Math::XYZPoint getLocalReferencePoint() const;

        /**
         * @brief Check if this MCParticle is a primary particle by checking if the parent particle reference is a nullptr.
         * @return Bollean flag indicating whether this MCParticle is a primary particle
         */
        bool isPrimary() const;

        /**
         * @brief Get PDG particle id for the particle
         * @return Particle id
         */
        int getParticleID() const;

        /**
         * @brief Get the arrival time for the particle
         * @return Arrival time of the particle in the respective sensor in global reference system
         */
        double getGlobalTime() const;

        /**
         * @brief Get the local time for the particle in the respective sensor
         * @return Arrival time of the particle in the respective sensor in the local system
         */
        double getLocalTime() const;

        /**
         * @brief Set the starting total energy of this particle in the respective sensor
         * @param total_energy Total energy of this particle at its start point
         */
        void setTotalEnergyStart(double total_energy);

        /**
         * @brief Return the starting total energy of this particle in the respective sensor
         * @return Total energy of this particle at its start point
         */
        double getTotalEnergyStart() const;

        /**
         * @brief Set the starting kinetic energy of this particle in the respective sensor
         * @param kinetic_energy Kinetic energy of this particle at its start point
         */
        void setKineticEnergyStart(double kinetic_energy);

        /**
         * @brief Return the starting kinetic energy of this particle in the respective sensor
         * @return Kinetic energy of this particle at its start point
         */
        double getKineticEnergyStart() const;

        /**
         * @brief Set the total number of charge carriers produced by this particle
         * @param total_charge Total charge deposited by this particle
         */
        void setTotalDepositedCharge(unsigned int total_charge);

        /**
         * @brief Return the total number of charge carriers deposited by this particle
         * @return Total number of deposited charge carriers
         */
        unsigned int getTotalDepositedCharge() const;

        /**
         * @brief Set the total energy deposited by this particle
         * @param total_energy Total energy deposited by this particle
         */
        void setTotalDepositedEnergy(double total_energy);

        /**
         * @brief Return the total energy deposited by this particle
         * @return Total energy deposited by this particle
         */
        double getTotalDepositedEnergy() const;

        /**
         * @brief Set the Monte-Carlo particle
         * @param mc_particle The Monte-Carlo particle
         * @warning Special method because parent can only be set after creation, should not be replaced later.
         */
        void setParent(const MCParticle* mc_particle);

        /**
         * @brief Get the parent MCParticle if it has one
         * @return Parent MCParticle or null pointer if it has no parent
         * @warning No \ref MissingReferenceException is thrown, because a particle without parent should always be handled.
         */
        const MCParticle* getParent() const;

        /**
         * @brief Get the primary MCParticle from which this MCParticle originates
         * @return Primary MCParticle. If it is a primary itself, returns pointer to self
         * @warning No \ref MissingReferenceException is thrown, because primary particles should always be handled.
         */
        const MCParticle* getPrimary() const;

        /**
         * @brief Set the MCParticle's track
         * @param mc_track The track
         * @warning Special method because track can only be set after creation, should not be replaced later.
         */
        void setTrack(const MCTrack* mc_track);

        /**
         * @brief Get the MCTrack of this MCParticle
         * @return Parent MCTrack or null pointer if it has no track
         * @warning No \ref MissingReferenceException is thrown, because a particle without a track should always be handled.
         */
        const MCTrack* getTrack() const;

        /**
         * @brief ROOT class definition
         */
        ClassDefOverride(MCParticle, 10); // NOLINT
        /**
         * @brief Default constructor for ROOT I/O
         */
        MCParticle() = default;

        /**
         * @brief Print an ASCII representation of MCParticle to the given stream
         * @param out Stream to print to
         */
        void print(std::ostream& out) const override;

        void loadHistory() override;
        void petrifyHistory() override;

    private:
        ROOT::Math::XYZPoint local_start_point_{};
        ROOT::Math::XYZPoint global_start_point_{};
        ROOT::Math::XYZPoint local_end_point_{};
        ROOT::Math::XYZPoint global_end_point_{};

        int particle_id_{};
        double local_time_{};
        double global_time_{};
        unsigned int deposited_charge_{};
        double deposited_energy_{};
        double total_energy_start_{};
        double kinetic_energy_start_{};

        PointerWrapper<MCParticle> parent_;
        PointerWrapper<MCTrack> track_;
    };

    /**
     * @brief Typedef for message carrying MC particles
     */
    using MCParticleMessage = Message<MCParticle>;
} // namespace allpix

#endif /* ALLPIX_MC_PARTICLE_H */
