/**
 * @author Mathieu Benoit <benoit@lal.in2p3.fr>
 */

#include "BumpsParameterizationG4.hpp"

#include <memory>

using namespace allpix;

BumpsParameterizationG4::BumpsParameterizationG4(const std::shared_ptr<PixelDetectorModel>& model)
    : model_(model), hsensorX(model->getHalfSensorSizeX()), hsensorY(model_->getHalfSensorSizeY()),
      hpixelX(model->getHalfPixelSizeX()), hpixelY(model->getHalfPixelSizeY()), npixelX(model->getNPixelsX()),
      npixelY(model->getNPixelsY()) {}

void BumpsParameterizationG4::ComputeTransformation(const G4int copyId, G4VPhysicalVolume* Bump) const {
    G4double XPos = posX(copyId) + model_->getBumpOffsetX();
    G4double YPos = posY(copyId) + model_->getBumpOffsetY();
    G4double ZPos = 0;

    Bump->SetTranslation(G4ThreeVector(XPos, YPos, ZPos));
    Bump->SetRotation(nullptr);
}

double BumpsParameterizationG4::posX(int id) const {
    G4int X = id % npixelX;
    return X * hpixelX * 2 + hpixelX - hsensorX;
}

double BumpsParameterizationG4::posY(int id) const {
    G4int Y = (id - (id % npixelX)) / npixelY;
    return Y * hpixelY * 2 + hpixelY - hsensorY;
}
