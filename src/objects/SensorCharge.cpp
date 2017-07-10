/**
 * @file
 * @brief Definition of object for charges in sensor
 * @copyright MIT License
 */

#include "SensorCharge.hpp"

using namespace allpix;

SensorCharge::SensorCharge(ROOT::Math::XYZPoint local_position,
                           ROOT::Math::XYZPoint global_position,
                           unsigned int charge,
                           double event_time)
    : local_position_(std::move(local_position)), global_position_(std::move(global_position)), charge_(charge),
      event_time_(event_time) {}

ROOT::Math::XYZPoint SensorCharge::getLocalPosition() const {
    return local_position_;
}

unsigned int SensorCharge::getCharge() const {
    return charge_;
}

double SensorCharge::getEventTime() const {
    return event_time_;
}

ClassImp(SensorCharge)
