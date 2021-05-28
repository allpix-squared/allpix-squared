/**
 * @file
 * @brief Implementation of the common MuPixModel class
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "MuPixModel.hpp"

#include <numeric>

#include "core/messenger/Messenger.hpp"

using namespace allpix::mupix;

MuPixModel::MuPixModel(Configuration& config) {
    // Set default values
    config.setDefault<double>("threshold", Units::get(30, "mV"));
    config.setDefault<double>("clock_bin_ts1", Units::get(8, "ns"));
    config.setDefault<double>("clock_bin_ts2", Units::get(128, "ns"));
    config.setDefault<double>("integration_time", Units::get(2, "us"));
    config.setDefault<double>("ts2_integration_time", Units::get(2, "us"));

    // Get config value
    threshold_ = config.get<double>("threshold");
    ts1_clock_ = config.get<double>("clock_bin_ts1");
    ts2_clock_ = config.get<double>("clock_bin_ts2");
    integration_time_ = config.get<double>("integration_time");
    ts2_integration_time_ = config.get<double>("ts2_integration_time");
}

std::vector<double> MuPixModel::amplify_pulse(const Pulse& pulse) const {
    LOG(ERROR) << "Reference amplification called";
    return std::vector<double>(1, 0.0);
}

std::tuple<bool, unsigned int> MuPixModel::get_ts1(double timestep, const std::vector<double>& pulse) const {
    LOG(TRACE) << "Calculating TS1";
    bool threshold_crossed = false;
    unsigned int ts1_clock_cycles = 0;

    // Lambda for threshold calculation
    auto calculate_is_above_threshold = [](double bin, double threshold) {
        return (threshold > 0. ? bin > threshold : bin < threshold);
    };

    // Find the point where the signal crosses the threshold
    auto max_ts1_clock_cycles = static_cast<unsigned int>(std::ceil(integration_time_ / ts1_clock_));
    while(ts1_clock_cycles < max_ts1_clock_cycles) {
        auto bin = pulse.at(static_cast<size_t>(std::floor(ts1_clock_cycles * ts1_clock_ / timestep)));
        if(calculate_is_above_threshold(bin, threshold_)) {
            threshold_crossed = true;
            break;
        };
        ts1_clock_cycles++;
    }
    return {threshold_crossed, ts1_clock_cycles};
}

unsigned int MuPixModel::get_ts2(unsigned int ts1, double timestep, const std::vector<double>& pulse) const {
    LOG(TRACE) << "Calculating TS2";
    bool was_above_treshold = true;
    auto ts2_clock_cycles = static_cast<unsigned int>(std::ceil(ts1 * ts1_clock_ / ts2_clock_));
    auto final_ts2_clock_cycles = ts2_clock_cycles;

    // Lambda for threshold calculation
    auto calculate_is_below_threshold = [](double bin, double threshold) {
        return (threshold > 0 ? bin < threshold : bin > threshold);
    };

    // Calculate max TS2 clock cycles after TS1 (+1), or cap at integration time
    auto max_ts2_time = std::min<double>(ts1 * ts1_clock_ + ts2_integration_time_, integration_time_);
    auto max_ts2_clock_cycles = static_cast<unsigned int>(std::ceil(max_ts2_time / ts2_clock_));

    while(ts2_clock_cycles < max_ts2_clock_cycles) {
        auto bin = pulse.at(static_cast<size_t>(std::floor(ts2_clock_cycles * ts2_clock_ / timestep)));
        auto is_below_threshold = calculate_is_below_threshold(bin, threshold_);
        if(was_above_treshold && is_below_threshold) {
            final_ts2_clock_cycles = ts2_clock_cycles;
            was_above_treshold = false;
        } else if(!was_above_treshold && !is_below_threshold) {
            was_above_treshold = true;
        }
        ts2_clock_cycles++;
    }

    // Cap TS2 if pulse is above threshold at the end
    if(was_above_treshold) {
        final_ts2_clock_cycles = max_ts2_clock_cycles - 1;
    }

    return final_ts2_clock_cycles;
}
