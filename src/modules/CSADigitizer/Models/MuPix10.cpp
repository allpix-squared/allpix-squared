/**
 * @file
 * @brief Implementation of the MuPix10 CSADigitizerModel class
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "MuPix10.hpp"

#include <algorithm>
#include <iterator>

#include "core/utils/unit.h"

#include "tools/ROOT.h"

using namespace allpix::csa;

void MuPix10::configure(Configuration& config) {
    // Call base class configuration
    CSADigitizerModel::configure(config);

    // Set default parameters
    config.setDefault<double>("parameter_amplification", Units::get(2.51424577e+14, "V/C"));
    config.setDefault<double>("parameter_rise", Units::get(3.35573247e-07, "s"));
    config.setDefault<double>("parameter_fall", Units::get(1.85969061e+04, "V/s"));

    // Get parameters
    A_ = config.get<double>("parameter_amplification");
    R_ = config.get<double>("parameter_rise");
    F_ = config.get<double>("parameter_fall");

    LOG(DEBUG) << "Parameters: A = " << Units::display(A_, {"V/C", "fC/mV"})
               << ", R = " << Units::display(R_, {"ns", "us", "ms", "s"})
               << ", F = " << Units::display(F_, {"V/s", "mV/ns"});
}

std::vector<double> MuPix10::amplify_pulse(const Pulse& pulse) const {
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
