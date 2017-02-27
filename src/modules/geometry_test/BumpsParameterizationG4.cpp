/**
 * @author Mathieu Benoit <benoit@lal.in2p3.fr>
 */

#include "BumpsParameterizationG4.hpp"

using namespace allpix;

BumpsParameterizationG4::BumpsParameterizationG4(std::shared_ptr<PixelDetectorModel> model): model_(model) {
    hsensorX=model_->GetHalfSensorX();
    hsensorY=model_->GetHalfSensorY();
    hpixelX=model_->GetHalfPixelX();
    hpixelY=model_->GetHalfPixelY();
    
    npixelX = model_->GetNPixelsX();
    npixelY = model_->GetNPixelsY();
}



void BumpsParameterizationG4::ComputeTransformation(G4int copyId,
                                                    G4VPhysicalVolume* Bump) const {
    G4double XPos = posX(copyId) + model_->GetBumpOffsetX();
    G4double YPos = posY(copyId) + model_->GetBumpOffsetY();
    G4double ZPos = 0;
    
    Bump->SetTranslation(G4ThreeVector(XPos,YPos,ZPos));
    Bump->SetRotation(0);
    
}

double BumpsParameterizationG4::posX(int id) const {
    G4int X =  id%npixelX;
    return X*hpixelX*2 + hpixelX - hsensorX;
}

double BumpsParameterizationG4::posY(int id) const {
    G4int Y = (id-(id%npixelX))/npixelY;
    return Y*hpixelY*2 + hpixelY - hsensorY;
}
                                                
