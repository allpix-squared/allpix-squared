/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "SensorCharge.hpp"

using namespace allpix;

SensorCharge::SensorCharge(ROOT::Math::XYZPoint position, unsigned int charge, long double event_time)
    : position_(std::move(position)), charge_(charge), event_time_(event_time) {}

ROOT::Math::XYZPoint SensorCharge::getPosition() const {
    return position_;
}

unsigned int SensorCharge::getCharge() const {
    return charge_;
}

long double SensorCharge::getEventTime() const {
    return event_time_;
}
