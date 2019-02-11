/**
 * @file
 * @brief Implementation of pulse object
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Pulse.hpp"
#include "exceptions.h"

#include <cmath>

using namespace allpix;

Pulse::Pulse(double integration_time, double time_bin) : time_(integration_time), bin_(time_bin) {
    // Prepare storage
    auto bins = static_cast<size_t>(std::ceil(time_ / bin_));
    pulse_.resize(bins);
}

Pulse::Pulse() : Pulse(1, 1) {}

void Pulse::addCharge(double charge, double time) {
    auto bin = static_cast<size_t>(time / bin_);
    if(bin < pulse_.size()) {
        pulse_.at(bin) += charge;
    }
}

unsigned int Pulse::getCharge() const {
    double charge = 0;
    for(auto& i : pulse_) {
        charge += i;
    }
    return static_cast<unsigned int>(charge);
}

const std::vector<double>& Pulse::getPulse() const {
    return pulse_;
}

double Pulse::getBinning() const {
    return bin_;
}

Pulse& Pulse::operator+=(const Pulse& rhs) {
    auto rhs_pulse = rhs.getPulse();

    if(this->getBinning() != rhs.getBinning()) {
        throw IncompatibleDatatypesException(typeid(*this), typeid(rhs), "different time binning");
    } else {
        // If new pulse is longer, extend:
        if(this->pulse_.size() < rhs_pulse.size()) {
            this->pulse_.resize(rhs_pulse.size());
        }

        // Add up the individual bins:
        for(size_t bin = 0; bin < rhs_pulse.size(); bin++) {
            this->pulse_.at(bin) += rhs_pulse.at(bin);
        }
    }
    return *this;
}
