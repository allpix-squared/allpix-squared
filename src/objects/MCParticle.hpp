/**
 * @file
 * @brief Definition of Monte-Carlo particle object
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MC_PARTICLE_H
#define ALLPIX_MC_PARTICLE_H

#include <Math/Point3D.h>
#include <TRef.h>

#include "MCTrack.hpp"
#include "Object.hpp"

namespace allpix {
    /**
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
         * @param time The arrival time of the particle in the sensor
         */
        MCParticle(ROOT::Math::XYZPoint local_start_point,
                   ROOT::Math::XYZPoint global_start_point,
                   ROOT::Math::XYZPoint local_end_point,
                   ROOT::Math::XYZPoint global_end_point,
                   int particle_id,
                   double time);

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
         * @brief Get PDG particle id for the particle
         * @return Particle id
         */
        int getParticleID() const;

        /**
         * @brief Get the arrival time for the particle
         * @return Arrival time of the particle in the respective sensor
         */
        double getTime() const;

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
        ClassDefOverride(MCParticle, 7);
        /**
         * @brief Default constructor for ROOT I/O
         */
        MCParticle() = default;

        /**
         * @brief Print an ASCII representation of MCParticle to the given stream
         * @param out Stream to print to
         */
        void print(std::ostream& out) const override;

        void petrifyHistory() override;

    private:
        ROOT::Math::XYZPoint local_start_point_{};
        ROOT::Math::XYZPoint global_start_point_{};
        ROOT::Math::XYZPoint local_end_point_{};
        ROOT::Math::XYZPoint global_end_point_{};

        int particle_id_{};
        double time_{};

        PointerWrapper<MCParticle> parent_;
        PointerWrapper<MCTrack> track_;
    };

    /**
     * @brief Typedef for message carrying MC particles
     */
    using MCParticleMessage = Message<MCParticle>;
} // namespace allpix

#endif /* ALLPIX_MC_PARTICLE_H */
