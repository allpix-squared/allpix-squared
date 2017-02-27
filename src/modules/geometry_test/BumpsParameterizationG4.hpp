/**
 * @author Mathieu Benoit <benoit@lal.in2p3.fr>
 */

#ifndef ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_BUMP_PARAMETERIZATION_HH_
#define ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_BUMP_PARAMETERIZATION_HH_

#include <memory>

#include "G4VPVParameterisation.hh"
#include "G4ThreeVector.hh"
#include "G4VPhysicalVolume.hh"

#include "../../core/geometry/PixelDetectorModel.hpp"

namespace allpix {
    class BumpsParameterizationG4 : public G4VPVParameterisation {
    public:
        BumpsParameterizationG4(std::shared_ptr<PixelDetectorModel> model);
        
        void ComputeTransformation(G4int, G4VPhysicalVolume*) const;
        double posX(int id) const;
        double posY(int id) const;
        
    private:
        G4double hsensorX;
        G4double hsensorY;
        G4double hpixelX;
        G4double hpixelY;
        
        G4int npixelX;
        G4int npixelY;
        
        std::shared_ptr<PixelDetectorModel> model_;
    };
}

#endif /* ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_BUMP_PARAMETERIZATION_HH_ */
