/**
 * @file
 * @brief Definition of MuPix10 with second threshold for the MuPix digitizer module
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "MuPix10Double.hpp"

#include <cmath>

using namespace allpix::mupix;

MuPix10Double::MuPix10Double(Configuration& config) : MuPix10(config) {
    // Set default config values
    config.setDefault<double>("threshold_high", Units::get(40, "mV"));

    // Get config values
    threshold_high_ = config.get<double>("threshold_high");
}

std::tuple<bool, unsigned int> MuPix10Double::get_ts1(double timestep, const std::vector<double>& pulse) const {
    auto th_low_result = MuPix10::get_ts1(timestep, pulse);

    if(!std::get<0>(th_low_result)) {
        return th_low_result;
    }

    // Lambda for threshold calculation
    auto calculate_is_above_threshold = [](double bin, double threshold) {
        return (threshold > 0. ? bin > threshold : bin < threshold);
    };

    // Same as MuPixModel::get_ts1, just for high threshold
    bool th_high_crossed = false;
    auto ts1_th_high_clock_cycles = std::get<1>(th_low_result);
    auto max_ts1_th_high_clock_cycles = static_cast<unsigned int>(std::ceil(integration_time_ / ts1_clock_));
    while(ts1_th_high_clock_cycles < max_ts1_th_high_clock_cycles) {
        auto bin = pulse.at(static_cast<size_t>(std::floor(ts1_th_high_clock_cycles * ts1_clock_ / timestep)));
        if(calculate_is_above_threshold(bin, threshold_high_)) {
            th_high_crossed = true;
            break;
        };
        ts1_th_high_clock_cycles++;
    }
    return {th_high_crossed, std::get<1>(th_low_result)};
}
