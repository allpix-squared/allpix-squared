/**
 * @file
 * @brief Definition of MuPix10 for the MuPix digitizer module
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "MuPix10.hpp"

#include <numeric>

#include "tools/ROOT.h"

using namespace allpix::mupix;

MuPix10::MuPix10(Configuration& config) : MuPixModel(config) {
    // Set default parameters
    config.setDefaultArray<double>("parameters",
                                   {Units::get(4.2e+14, "V/C"), Units::get(1.1e-07, "s"), Units::get(7.6e+04, "V/s")});

    // Get parameters
    auto parameters = config.getArray<double>("parameters");

    // Check that there are exactly three parameters
    if(parameters.size() != 3) {
        throw InvalidValueError(
            config,
            "parameters",
            "The number of parameters does not line up with the amount of parameters for the MuPix10 (3).");
    }

    A_ = parameters[0];
    R_ = parameters[1];
    F_ = parameters[2];
}

std::vector<double> MuPix10::amplify_pulse(const Pulse& pulse) const {
    LOG(TRACE) << "Amplifying pulse";

    auto pulse_vec = pulse.getPulse();
    auto charge = pulse.getCharge();
    auto timestep = pulse.getBinning();
    auto ntimepoints = static_cast<size_t>(ceil(integration_time_ / timestep));
    std::vector<double> amplified_pulse_vec(ntimepoints);

    // Assume pulse is delta peak with all charges at the maximum
    auto kmin = static_cast<size_t>(std::distance(pulse_vec.begin(), std::max_element(pulse_vec.begin(), pulse_vec.end())));
    LOG(INFO) << "kmin=" << kmin; // DEBUG just for now

    // Set output to zero before pulse arrives
    for(size_t k = 0; k < kmin; ++k) {
        amplified_pulse_vec.at(k) = 0.0;
    }
    // Calculate output starting relative to arrival
    for(size_t k = kmin; k < ntimepoints; ++k) {
        auto time = static_cast<double>(k - kmin) * timestep;
        auto out = charge * A_ * (1 - exp(-time / R_)) - F_ * time;
        amplified_pulse_vec.at(k) = (out < 0.0 ? 0.0 : out);
    }

    return amplified_pulse_vec;
}
