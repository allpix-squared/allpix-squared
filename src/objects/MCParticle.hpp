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
         * @param entry_point Entry point of the particle in the sensor
         * @param exit_point Exit point of the particle in the sensor
         * @param particle_id Identifier for the particle type
         */
        MCParticle(ROOT::Math::XYZPoint entry_point, ROOT::Math::XYZPoint exit_point, int particle_id);

        /**
         * @brief Get the entry point of the particle
         * @return Particle entry point
         */
        ROOT::Math::XYZPoint getEntryPoint() const;
        /**
         * @brief Get the exit point of the particle
         * @return Particle exit point
         */
        ROOT::Math::XYZPoint getExitPoint() const;
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
        ROOT::Math::XYZPoint entry_point_{};
        ROOT::Math::XYZPoint exit_point_{};

        int particle_id_{};
    };

    /**
     * @brief Typedef for message carrying MC particles
     */
    using MCParticleMessage = Message<MCParticle>;
} // namespace allpix

#endif
