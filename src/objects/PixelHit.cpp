/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "PixelHit.hpp"

using namespace allpix;

PixelHit::PixelHit(PixelHit::Pixel pixel) : pixel_(std::move(pixel)) {}

PixelHit::Pixel PixelHit::getPixel() const {
    return pixel_;
}

ClassImp(PixelHit)
