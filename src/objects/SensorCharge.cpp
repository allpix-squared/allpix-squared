/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "SensorCharge.hpp"

using namespace allpix;

SensorCharge::SensorCharge(ROOT::Math::XYZPoint position, unsigned int charge, double event_time)
    : position_(std::move(position)), charge_(charge), event_time_(event_time) {}
SensorCharge::~SensorCharge() = default;

SensorCharge::SensorCharge(const SensorCharge&) = default;
SensorCharge& SensorCharge::operator=(const SensorCharge&) = default;

SensorCharge::SensorCharge(SensorCharge&& other) noexcept
    : position_(std::move(other.position_)), charge_(other.charge_), event_time_(other.event_time_) {}
SensorCharge& SensorCharge::operator=(SensorCharge&& other) noexcept {
    position_ = std::move(other.position_);
    charge_ = other.charge_;
    event_time_ = other.event_time_;
    return *this;
}

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
