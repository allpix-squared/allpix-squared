/**
 * @file
 * @brief Implementation of pulse object
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Pulse.hpp"

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
