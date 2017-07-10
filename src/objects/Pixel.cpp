/**
 * @file
 * @brief Implementation of pixel object
 * @copyright MIT License
 */

#include "Pixel.hpp"

using namespace allpix;

Pixel::Pixel(Pixel::Index index,
             ROOT::Math::XYZPoint local_center,
             ROOT::Math::XYZPoint global_center,
             ROOT::Math::XYVector size)
    : index_(std::move(index)), local_center_(std::move(local_center)), global_center_(std::move(global_center)),
      size_(size) {}

Pixel::Index Pixel::getIndex() const {
    return index_;
}

ROOT::Math::XYZPoint Pixel::getLocalCenter() const {
    return local_center_;
}
ROOT::Math::XYZPoint Pixel::getGlobalCenter() const {
    return global_center_;
}
ROOT::Math::XYVector Pixel::getSize() const {
    return size_;
}

ClassImp(Pixel)
