/**
 * Message for a particle passing through the sensor
 *
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MC_PARTICLE_H
#define ALLPIX_MC_PARTICLE_H

#include <Math/Point3D.h>

#include "Object.hpp"

namespace allpix {
    // object definition
    class MCParticle : public Object {
    public:
        explicit MCParticle(ROOT::Math::XYZPoint entry_point, ROOT::Math::XYZPoint exit_point, int particleID);

        ROOT::Math::XYZPoint getEntryPoint() const;
        ROOT::Math::XYZPoint getExitPoint() const;
        int getParticleID() const;

        ClassDef(MCParticle, 1);
        MCParticle() = default;

    private:
        ROOT::Math::XYZPoint entry_point_{};
        ROOT::Math::XYZPoint exit_point_{};

        int particle_id_{};
    };

    // link to the carrying message
    using MCParticleMessage = Message<MCParticle>;
} // namespace allpix

#endif
