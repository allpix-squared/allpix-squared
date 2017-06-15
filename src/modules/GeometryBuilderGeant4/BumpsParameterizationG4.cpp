/**
 * @author Mathieu Benoit <benoit@lal.in2p3.fr>
 */

#include "BumpsParameterizationG4.hpp"

#include <memory>

using namespace allpix;

BumpsParameterizationG4::BumpsParameterizationG4(const std::shared_ptr<HybridPixelDetectorModel>& model)
    : model_(model), hsensorX(model->getSensorSize().x() / 2.0), hsensorY(model_->getSensorSize().y() / 2.0),
      hpixelX(model->getPixelSize().x() / 2.0), hpixelY(model->getPixelSize().y() / 2.0), npixelX(model->getNPixels().x()),
      npixelY(model->getNPixels().y()) {}

void BumpsParameterizationG4::ComputeTransformation(const G4int copyId, G4VPhysicalVolume* Bump) const {
    G4double XPos = posX(copyId) + model_->getBumpOffset().x();
    G4double YPos = posY(copyId) + model_->getBumpOffset().y();
    G4double ZPos = 0;

    Bump->SetTranslation(G4ThreeVector(XPos, YPos, ZPos));
    Bump->SetRotation(nullptr);
}

double BumpsParameterizationG4::posX(int id) const {
    G4int X = id % model_->getNPixels().x();
    return X * model_->getPixelSize().x() / 2.0 * 2 + model_->getPixelSize().x() / 2.0 - model_->getSensorSize().x() / 2.0;
}

double BumpsParameterizationG4::posY(int id) const {
    G4int Y = (id - (id % model_->getNPixels().x())) / model_->getNPixels().y();
    return Y * model_->getPixelSize().y() / 2.0 * 2 + model_->getPixelSize().y() / 2.0 - model_->getSensorSize().y() / 2.0;
}
