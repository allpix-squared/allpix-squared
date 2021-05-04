/**
 * @file
 * @brief Implementation of the CSADigitizerModel class
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "CSADigitizerModel.hpp"

#include <numeric>

#include "core/messenger/Messenger.hpp"

using namespace allpix::csa;

void CSADigitizerModel::configure(Configuration& config) {
    // Copy relevant config values
    threshold_ = config.get<double>("threshold");
    integration_time_ = config.get<double>("integration_time");
    ignore_polarity_ = config.get<bool>("ignore_polarity");
    if(config.has("clock_bin_ts1")) {
        clockTS1_ = config.get<double>("clock_bin_ts1");
    }
    if(config.has("clock_bin_ts2")) {
        clockTS2_ = config.get<double>("clock_bin_ts2");
    }
}

std::vector<double> CSADigitizerModel::amplify_pulse(const Pulse&) const {
    // No reference implementation
    return std::vector<double>(1, 0.0);
}

std::tuple<bool, unsigned int> CSADigitizerModel::get_ts1(double timestep, const std::vector<double>& pulse) const {
    LOG(TRACE) << "Calculating TS1";
    bool threshold_crossed{false};
    unsigned int ts1_clock_cycles{0};

    // Find the clock cycle where the signal first crosses above the threshold
    auto max_ts1_clock_cycles = static_cast<unsigned int>(std::ceil(integration_time_ / clockTS1_));
    while(ts1_clock_cycles < max_ts1_clock_cycles) {
        auto bin = pulse.at(static_cast<size_t>(std::floor(ts1_clock_cycles * clockTS1_ / timestep)));
        if(calculate_is_above_threshold(bin)) {
            threshold_crossed = true;
            break;
        };
        ts1_clock_cycles++;
    }
    return {threshold_crossed, ts1_clock_cycles};
}

unsigned int CSADigitizerModel::get_ts2(unsigned int ts1, double timestep, const std::vector<double>& pulse) const {
    LOG(TRACE) << "Calculating TS2, starting at " << Units::display(ts1 * clockTS1_, {"ps", "ns", "us"});

    // Start calculation from the next ToT clock cycle following the threshold crossing
    auto ts2_clock_cycles = static_cast<unsigned int>(std::ceil(ts1 * clockTS1_ / clockTS2_));

    // Find the point where the signal first crosses below the threshold
    auto max_ts2_clock_cycles = static_cast<unsigned int>(std::ceil(integration_time_ / clockTS2_));
    while(ts2_clock_cycles < max_ts2_clock_cycles) {
        auto bin = pulse.at(static_cast<size_t>(std::floor(ts2_clock_cycles * clockTS2_ / timestep)));
        if(calculate_is_below_threshold(bin)) {
            break;
        }
        ts2_clock_cycles++;
    }

    return ts2_clock_cycles;
}

std::tuple<bool, double> CSADigitizerModel::get_arrival(double timestep, const std::vector<double>& pulse) const {
    LOG(TRACE) << "Calculating arrival time";
    bool threshold_crossed{false};
    double time{0.0};

    // Find the time where the signal first crosses above the threshold
    while(time < integration_time_) {
        auto bin = pulse.at(static_cast<size_t>(std::floor(time / timestep)));
        if(calculate_is_above_threshold(bin)) {
            threshold_crossed = true;
            break;
        };
        time += timestep;
    }
    return {threshold_crossed, time};
}

double CSADigitizerModel::get_pulse_integral(const std::vector<double>& pulse) const {
    return std::accumulate(pulse.begin(), pulse.end(), 0.0);
}

bool CSADigitizerModel::calculate_is_below_threshold(double voltage) const {
    if(ignore_polarity_) {
        return (std::fabs(voltage) < std::fabs(threshold_));
    } else {
        return (threshold_ > 0.0 ? voltage < threshold_ : voltage > threshold_);
    }
}

bool CSADigitizerModel::calculate_is_above_threshold(double voltage) const {
    if(ignore_polarity_) {
        return (std::fabs(voltage) > std::fabs(threshold_));
    } else {
        return (threshold_ > 0.0 ? voltage > threshold_ : voltage < threshold_);
    }
}
