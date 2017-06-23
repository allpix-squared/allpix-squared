#include "Parameterization2DG4.hpp"

#include <memory>

using namespace allpix;

Parameterization2DG4::Parameterization2DG4(
    int div_x, double size_x, double size_y, double offset_x, double offset_y, double pos_z)
    : div_x_(div_x), size_x_(size_x), size_y_(size_y), offset_x_(offset_x), offset_y_(offset_y), pos_z_(pos_z) {}

void Parameterization2DG4::ComputeTransformation(int copy_id, G4VPhysicalVolume* phys_volume) const {
    auto idx_x = copy_id % div_x_;
    auto idx_y = copy_id / div_x_;

    auto pos_x = (idx_x + 0.5) * size_x_ + offset_x_;
    auto pos_y = (idx_y + 0.5) * size_y_ + offset_y_;

    phys_volume->SetTranslation(G4ThreeVector(pos_x, pos_y, pos_z_));
    phys_volume->SetRotation(nullptr);
}
