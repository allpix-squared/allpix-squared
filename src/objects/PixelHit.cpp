/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "PixelHit.hpp"

using namespace allpix;

PixelHit::PixelHit(Pixel pixel, double time, double signal) : pixel_(std::move(pixel)), time_(time), signal_(signal) {}

PixelHit::Pixel PixelHit::getPixel() const {
    return pixel_;
}

ClassImp(PixelHit)
