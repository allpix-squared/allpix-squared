/**
 * @author Mathieu Benoit <benoit@lal.in2p3.fr>
 */

#include "BumpsParameterizationG4.hpp"

#include <memory>

using namespace allpix;

BumpsParameterizationG4::BumpsParameterizationG4(const std::shared_ptr<HybridPixelDetectorModel>& model)
    : model_(model), hsensorX(model->getSensorSize().x() / 2.0), hsensorY(model_->getSensorSize().y() / 2.0),
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
    G4int X = id % model_->getNPixelsX();
    return X * model_->getHalfPixelSizeX() * 2 + model_->getHalfPixelSizeX() - model_->getSensorSize().x() / 2.0;
}

double BumpsParameterizationG4::posY(int id) const {
    G4int Y = (id - (id % model_->getNPixelsX())) / model_->getNPixelsY();
    return Y * model_->getHalfPixelSizeY() * 2 + model_->getHalfPixelSizeY() - model_->getSensorSize().y() / 2.0;
}
