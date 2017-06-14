/**
 * @author Mathieu Benoit <benoit@lal.in2p3.fr>
 */

#ifndef ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_BUMP_PARAMETERIZATION_HH_
#define ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_BUMP_PARAMETERIZATION_HH_

#include <memory>

#include <G4ThreeVector.hh>
#include <G4VPVParameterisation.hh>
#include <G4VPhysicalVolume.hh>

#include "core/geometry/HybridPixelDetectorModel.hpp"

namespace allpix {
    class BumpsParameterizationG4 : public G4VPVParameterisation {
    public:
        explicit BumpsParameterizationG4(const std::shared_ptr<HybridPixelDetectorModel>& model);

        void ComputeTransformation(const G4int, G4VPhysicalVolume*) const override;
        double posX(int id) const;
        double posY(int id) const;

    private:
        std::shared_ptr<HybridPixelDetectorModel> model_;

        G4double hsensorX;
        G4double hsensorY;
        G4double hpixelX;
        G4double hpixelY;

        G4int npixelX;
        G4int npixelY;
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_BUMP_PARAMETERIZATION_HH_ */
