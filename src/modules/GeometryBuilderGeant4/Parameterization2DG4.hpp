#ifndef ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_PARAMETERIZATION_2D_HH_
#define ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_PARAMETERIZATION_2D_HH_

#include <memory>

#include <G4ThreeVector.hh>
#include <G4VPVParameterisation.hh>
#include <G4VPhysicalVolume.hh>

namespace allpix {
    class Parameterization2DG4 : public G4VPVParameterisation {
    public:
        explicit Parameterization2DG4(
            int div_x, double size_x, double size_y, double offset_x, double offset_y, double pos_z);

        void ComputeTransformation(int, G4VPhysicalVolume*) const override;

    private:
        int div_x_;
        double size_x_;
        double size_y_;
        double offset_x_;
        double offset_y_;
        double pos_z_;
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_PARAMETERIZATION_2D_HH_ */
