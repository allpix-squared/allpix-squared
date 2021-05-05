/**
 * @file
 * @brief Implementation of the Simple CSADigitizerModel class
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "SimpleModel.hpp"

#include "tools/ROOT.h"

using namespace allpix::csa;

void SimpleModel::configure(Configuration& config) {
    CSADigitizerModel::configure(config);

    config.setDefault<double>("impulse_response_timestep", Units::get(0.01, "ns")); // FIXME: get from PulseTranfer (?)
    config.setDefault<double>("feedback_capacitance", Units::get(5e-15, "C/V"));
    config.setDefault<double>("rise_time_constant", Units::get(1e-9, "s"));
    config.setDefault<double>("feedback_time_constant", Units::get(10e-9, "s")); // R_f * C_f

    timestep_ = config.get<double>("impulse_response_timestep");
    tauF_ = config.get<double>("feedback_time_constant");
    tauR_ = config.get<double>("rise_time_constant");
    auto capacitance_feedback = config.get<double>("feedback_capacitance");
    resistance_feedback_ = tauF_ / capacitance_feedback;

    LOG(DEBUG) << "Parameters: cf = " << Units::display(capacitance_feedback, {"C/V", "fC/mV"})
               << ", rf = " << Units::display(resistance_feedback_, "V*s/C")
               << ", tauF = " << Units::display(tauF_, {"ns", "us", "ms", "s"})
               << ", tauR = " << Units::display(tauR_, {"ns", "us", "ms", "s"});

    precalculate_impulse_response();
}

void SimpleModel::precalculate_impulse_response() {
    auto ntimepoints = static_cast<size_t>(std::ceil(integration_time_ / timestep_));
    impulse_response_.reserve(ntimepoints);

    for(size_t k = 0; k < ntimepoints; ++k) {
        auto time = static_cast<double>(k) * timestep_;
        impulse_response_.push_back(resistance_feedback_ * (exp(-time / tauF_) - exp(-time / tauR_)) / (tauF_ - tauR_));
    }

    LOG(INFO) << "Initialized impulse response with timestep " << Units::display(timestep_, {"ps", "ns", "us"})
              << " and integration time " << Units::display(integration_time_, {"ns", "us", "ms"})
              << ", samples: " << ntimepoints;
}

std::vector<double> SimpleModel::amplify_pulse(const Pulse& pulse) const {
    auto pulse_vec = pulse.getPulse();
    auto timestep = pulse.getBinning();
    auto ntimepoints = static_cast<size_t>(ceil(integration_time_ / timestep));
    std::vector<double> amplified_pulse_vec(ntimepoints);
    auto input_length = pulse_vec.size();
    // convolution of the pulse (size input_length) with the impulse response (size ntimepoints)
    for(size_t k = 0; k < ntimepoints; ++k) {
        double outsum{};
        // convolution: multiply pulse_vec.at(k - i) * impulse_response_.at(i), when (k - i) < input_length
        // -> no point to start i at 0, start from jmin:
        size_t jmin = (k >= input_length - 1) ? k - (input_length - 1) : 0;
        for(size_t i = jmin; i <= k; ++i) {
            if((k - i) < input_length) {
                // FIXME: std::round or something like that?
                auto time_index = i * static_cast<size_t>(timestep / timestep_);
                outsum += pulse_vec.at(k - i) * impulse_response_.at(time_index);
            }
        }
        amplified_pulse_vec.at(k) = outsum;
    }

    return amplified_pulse_vec;
}
