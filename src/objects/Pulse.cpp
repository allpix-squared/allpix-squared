/**
 * @file
 * @brief Implementation of pulse object
 *
 * @copyright Copyright (c) 2019-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "Pulse.hpp"

#include "objects/exceptions.h"

#include <cmath>
#include <numeric>

using namespace allpix;

Pulse::Pulse(double time_bin) noexcept : bin_(time_bin), initialized_(true) {}

Pulse::Pulse(double time_bin, double total_time) : bin_(time_bin), initialized_(true) {
    auto bins = static_cast<size_t>(std::lround(total_time / bin_));
    try {
        this->reserve(bins);
    } catch(const std::bad_alloc& e) {
        PulseBadAllocException(bins, total_time, e.what());
    }
}

void Pulse::addCharge(double charge, double time) {
    // For uninitialized pulses, store all charge in the first bin:
    auto bin = (initialized_ ? static_cast<size_t>(std::lround(time / bin_)) : 0);

    try {
        // Adapt pulse storage vector:
        if(bin >= this->size()) {
            this->resize(bin + 1);
        }
        this->at(bin) += charge;
    } catch(const std::bad_alloc& e) {
        PulseBadAllocException(bin + 1, time, e.what());
    }
}

int Pulse::getCharge() const {
    double charge = std::accumulate(this->begin(), this->end(), 0.0);
    return static_cast<int>(std::lround(charge));
}

double Pulse::getBinning() const { return bin_; }

bool Pulse::isInitialized() const { return initialized_; }

Pulse& Pulse::operator+=(const Pulse& rhs) {
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
    if(this->size() < rhs.size()) {
        this->resize(rhs.size());
    }

    // Add up the individual bins:
    for(size_t bin = 0; bin < rhs.size(); bin++) {
        this->at(bin) += rhs.at(bin);
    }

    return *this;
}
