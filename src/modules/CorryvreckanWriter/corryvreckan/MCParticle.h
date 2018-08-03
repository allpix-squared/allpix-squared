#ifndef MCPARTICLE_H
#define MCPARTICLE_H 1

#include <Math/Point3D.h>
#include "Object.hpp"

namespace corryvreckan {

    class MCParticle : public Object {

    public:
        // Constructors and destructors
        MCParticle() = default;
        MCParticle(std::string detectorID,
                   int particle_id,
                   ROOT::Math::XYZPoint local_start_point,
                   ROOT::Math::XYZPoint local_end_point,
                   double timestamp)
            : Object(detectorID, timestamp), m_particle_id(particle_id), m_local_start_point(local_start_point),
              m_local_end_point(local_end_point) {}

        // Member variables
        int m_particle_id;
        ROOT::Math::XYZPoint m_local_start_point;
        ROOT::Math::XYZPoint m_local_end_point;

        // Member functions
        int getID() { return m_particle_id; }
        ROOT::Math::XYZPoint getLocalStart() { return m_local_start_point; }
        ROOT::Math::XYZPoint getLocalEnd() { return m_local_end_point; }

        // ROOT I/O class definition - update version number when you change this class!
        ClassDef(MCParticle, 2)
    };

    // Vector type declaration
    using MCParticles = std::vector<MCParticle*>;
} // namespace corryvreckan

#endif // MCPARTICLE_H
