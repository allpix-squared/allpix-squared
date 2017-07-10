/**
 * @file
 * @brief Definition of Monte-Carlo particle object
 * @copyright MIT License
 */

#ifndef ALLPIX_MC_PARTICLE_H
#define ALLPIX_MC_PARTICLE_H

#include <Math/Point3D.h>

#include "Object.hpp"

namespace allpix {
    /**
     * @brief Monte-Carlo particle through the sensor
     */
    class MCParticle : public Object {
    public:
        /**
         * @brief Construct a Monte-Carlo particle
         * @param local_entry_point Entry point of the particle in the sensor in local coordinates
         * @param global_entry_point Entry point of the particle in the sensor in global coordinates
         * @param local_exit_point Exit point of the particle in the sensor in local coordinates
         * @param global_exit_point Exit point of the particle in the sensor in global coordinates
         * @param particle_id Identifier for the particle type
         */
        MCParticle(ROOT::Math::XYZPoint local_entry_point,
                   ROOT::Math::XYZPoint global_entry_point,
                   ROOT::Math::XYZPoint local_exit_point,
                   ROOT::Math::XYZPoint global_exit_point,
                   int particle_id);

        /**
         * @brief Get the entry point of the particle in local coordinates
         * @return Particle entry point
         */
        ROOT::Math::XYZPoint getLocalEntryPoint() const;
        /**
         * @brief Get the entry point of the particle in global coordinates
         * @return Particle entry point
         */
        ROOT::Math::XYZPoint getGlobalEntryPoint() const;

        /**
         * @brief Get the exit point of the particle in local coordinates
         * @return Particle exit point
         */
        ROOT::Math::XYZPoint getLocalExitPoint() const;
        /**
         * @brief Get the entry point of the particle in global coordinates
         * @return Particle entry point
         */
        ROOT::Math::XYZPoint getGlobalExitPoint() const;

        /**
         * @brief Get particle identifier
         * @return Particle identifier
         */
        int getParticleID() const;

        /**
         * @brief ROOT class definition
         */
        ClassDef(MCParticle, 1);
        /**
         * @brief Default constructor for ROOT I/O
         */
        MCParticle() = default;

    private:
        ROOT::Math::XYZPoint local_entry_point_{};
        ROOT::Math::XYZPoint global_entry_point_{};
        ROOT::Math::XYZPoint local_exit_point_{};
        ROOT::Math::XYZPoint global_exit_point_{};

        int particle_id_{};
    };

    /**
     * @brief Typedef for message carrying MC particles
     */
    using MCParticleMessage = Message<MCParticle>;
} // namespace allpix

#endif
