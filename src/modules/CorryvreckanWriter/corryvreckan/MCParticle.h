#ifndef MCPARTICLE_H
#define MCPARTICLE_H 1

#include <Math/Point3D.h>
#include "TestBeamObject.h"

namespace corryvreckan {

    class MCParticle : public TestBeamObject {

    public:
        // Constructors and destructors
        MCParticle() = default;
        MCParticle(int particle_id, ROOT::Math::XYZPoint local_start_point, ROOT::Math::XYZPoint local_end_point) {
            m_particle_id = particle_id;
            m_local_start_point = local_start_point;
            m_local_end_point = local_end_point;
        }

        // Member variables
        int m_particle_id;
        ROOT::Math::XYZPoint m_local_start_point;
        ROOT::Math::XYZPoint m_local_end_point;

        // Member functions
        int getID() { return m_particle_id; }
        ROOT::Math::XYZPoint getLocalStart() { return m_local_start_point; }
        ROOT::Math::XYZPoint getLocalEnd() { return m_local_end_point; }

        // ROOT I/O class definition - update version number when you change this class!
        ClassDef(MCParticle, 1)
    };

    // Vector type declaration
    using MCParticles = std::vector<MCParticle*>;
} // namespace corryvreckan

#endif // MCPARTICLE_H
