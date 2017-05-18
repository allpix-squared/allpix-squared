/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "SensorCharge.hpp"

using namespace allpix;

SensorCharge::SensorCharge(ROOT::Math::XYZPoint position, unsigned int charge)
    : position_(std::move(position)), charge_(charge) {}

ROOT::Math::XYZPoint SensorCharge::getPosition() const {
    return position_;
}

unsigned int SensorCharge::getCharge() const {
    return charge_;
}
