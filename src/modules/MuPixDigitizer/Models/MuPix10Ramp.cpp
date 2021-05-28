/**
 * @file
 * @brief Definition of MuPix10 in ramp mode for the MuPix digitizer module
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "MuPix10Ramp.hpp"

#include <cmath>

#include "core/messenger/Messenger.hpp"

using namespace allpix::mupix;

MuPix10Ramp::MuPix10Ramp(Configuration& config) : MuPix10(config) {
    threshold_slew_rate_ = config.get<double>("threshold_slew_rate");
}

unsigned int MuPix10Ramp::get_ts2(unsigned int ts1, double timestep, const std::vector<double>& pulse) const {
    // Same as MuPixModel::get_ts2, just with dynamic threshold
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
        auto dynamic_threshold = threshold_slew_rate_ * (ts2_clock_cycles * ts2_clock_ - ts1 * ts1_clock_);
        auto is_below_threshold = calculate_is_below_threshold(bin, dynamic_threshold);
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
