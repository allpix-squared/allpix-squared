/**
 * @file
 * @brief Implementation of pulse object
 * @copyright Copyright (c) 2018-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Pulse.hpp"
#include "objects/exceptions.h"

#include "objects/exceptions.h"

#include <cmath>
#include <numeric>

using namespace allpix;

Pulse::Pulse(double time_bin) : bin_(time_bin), initialized_(true) {}

void Pulse::addCharge(double charge, double time) {
    // For uninitialized pulses, store all charge in the first bin:
    auto bin = (initialized_ ? static_cast<size_t>(std::lround(time / bin_)) : 0);

    // Adapt pulse storage vector:
    if(bin >= pulse_.size()) {
        pulse_.resize(bin + 1);
    }
    pulse_.at(bin) += charge;
}

int Pulse::getCharge() const {
    double charge = std::accumulate(pulse_.begin(), pulse_.end(), 0.0);
    return static_cast<int>(std::round(charge));
}

const std::vector<double>& Pulse::getPulse() const {
    return pulse_;
}

double Pulse::getBinning() const {
    return bin_;
}

bool Pulse::isInitialized() const {
    return initialized_;
}

Pulse& Pulse::operator+=(const Pulse& rhs) {
    auto rhs_pulse = rhs.getPulse();

    // Allow to initialize uninitialized pulse
    if(!this->initialized_) {
        this->bin_ = rhs.getBinning();
        this->initialized_ = true;
    }

    // Check that the pulses are compatible by having the same binning:
    if(this->getBinning() != rhs.getBinning()) {
        throw IncompatibleDatatypesException(typeid(*this), typeid(rhs), "different time binning");
    }

    // If new pulse is longer, extend:
    if(this->pulse_.size() < rhs_pulse.size()) {
        this->pulse_.resize(rhs_pulse.size());
    }

    // Add up the individual bins:
    for(size_t bin = 0; bin < rhs_pulse.size(); bin++) {
        this->pulse_.at(bin) += rhs_pulse.at(bin);
    }

    return *this;
}
