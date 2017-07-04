/**
 * @file
 * @brief Definition of object for charges in sensor
 * @copyright MIT License
 */

#include "SensorCharge.hpp"

using namespace allpix;

SensorCharge::SensorCharge(ROOT::Math::XYZPoint position, unsigned int charge, double event_time)
    : position_(std::move(position)), charge_(charge), event_time_(event_time) {}

ROOT::Math::XYZPoint SensorCharge::getPosition() const {
    return position_;
}

unsigned int SensorCharge::getCharge() const {
    return charge_;
}

double SensorCharge::getEventTime() const {
    return event_time_;
}

ClassImp(SensorCharge)
