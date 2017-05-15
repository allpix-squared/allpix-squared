/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "PixelCharge.hpp"

using namespace allpix;

PixelCharge::PixelCharge(PixelCharge::Pixel pixel, unsigned int charge) : pixel_(std::move(pixel)), charge_(charge) {}

PixelCharge::Pixel PixelCharge::getPixel() const {
    return pixel_;
}

unsigned int PixelCharge::getCharge() const {
    return charge_;
}
