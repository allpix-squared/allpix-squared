/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "SensorCharge.hpp"

#include <utility>

using namespace allpix;

SensorCharge::SensorCharge(ROOT::Math::XYZVector position, unsigned int charge) : position_(position), charge_(charge) {}
SensorCharge::~SensorCharge() = default;

ROOT::Math::XYZPoint SensorCharge::getPosition() const {
    return position_;
}

unsigned int SensorCharge::getCharge() const {
    return charge_;
}
