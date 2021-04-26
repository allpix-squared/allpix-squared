/**
 * @file
 * @brief Definition of MuPix10 for the MuPix digitizer module
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "MuPix10.hpp"

#include <algorithm>
#include <iterator>
#include <numeric>

#include "tools/ROOT.h"

using namespace allpix::mupix;

MuPix10::MuPix10(Configuration& config) : MuPixModel(config) {
    // Set default parameters
    config.setDefault<double>("A_m", Units::get(0.01, "mV/e"));
    config.setDefault<double>("A_c", Units::get(90., "mV"));
    config.setDefault<double>("A_mu", Units::get(3400., "e"));
    config.setDefault<double>("t_R", Units::get(0.1, "us"));
    config.setDefault<double>("t_F", Units::get(4.8, "us"));
    config.setDefault<double>("t_S", Units::get(0.1, "us"));
    config.setDefault<double>("Fb", Units::get(23., "mV/us"));
    config.setDefault<double>("Fb_D", Units::get(2., "mV"));
    config.setDefault<double>("U_sat", Units::get(360., "mV"));
    config.setDefault<double>("pulse_cutoff_time", Units::get(1., "ns"));

    // Get parameters
    A_m_ = config.get<double>("A_m");
    A_c_ = config.get<double>("A_c");
    A_mu_ = config.get<double>("A_mu");
    t_R_ = config.get<double>("t_R");
    t_F_ = config.get<double>("t_F");
    t_S_ = config.get<double>("t_S");
    Fb_ = config.get<double>("Fb");
    Fb_D_ = config.get<double>("Fb_D");
    U_sat_ = config.get<double>("U_sat");
    pulse_cutoff_time_ = config.get<double>("pulse_cutoff_time");

    LOG(DEBUG) << "Initialized MuPix10 configuration with " << Units::display(pulse_cutoff_time_, "ns")
               << " charge pulse cutoff"
               << ", A_m=" << Units::display(A_m_, "mV/e") << ", A_c=" << Units::display(A_c_, "mV")
               << ", A_mu=" << Units::display(A_mu_, "e") << ", t_R=" << Units::display(t_R_, "us")
               << ", t_F=" << Units::display(t_F_, "us") << ", t_S=" << Units::display(t_S_, "us")
               << ", Fb=" << Units::display(Fb_, "mV/us") << ", Fb_D=" << Units::display(Fb_D_, "mV")
               << ", U_sat=" << Units::display(U_sat_, "mV");
}

std::vector<double> MuPix10::amplify_pulse(const Pulse& pulse) const {
    LOG(TRACE) << "Amplifying pulse";

    // Create output vector
    auto timestep = pulse.getBinning();
    auto max_pulse_bins = static_cast<ptrdiff_t>(pulse_cutoff_time_ / timestep);
    auto ntimepoints = static_cast<size_t>(ceil(integration_time_ / timestep));
    std::vector<double> amplified_pulse_vec(ntimepoints, 0.);

    // Cut pulse for specified time
    auto pulse_vec = pulse.getPulse();
    auto pulse_vec_first = std::find_if(pulse_vec.begin(), pulse_vec.end(), [](auto& c) { return c > 0.; });
    auto pulse_vec_last = (std::distance(pulse_vec_first, pulse_vec.end()) < max_pulse_bins)
                              ? pulse_vec.end()
                              : std::next(pulse_vec_first, max_pulse_bins);

    // Assume pulse is delta peak with all charges at the maximum
    auto charge = std::accumulate(pulse_vec_first, pulse_vec_last, 0.);
    auto kmin = static_cast<size_t>(std::distance(pulse_vec.begin(), pulse_vec_first));

    // Prepare variables for amplification
    double U_out = 0.;
    auto A = (A_m_ * charge + A_c_) * (1. - exp(-charge / A_mu_));
    double rcshaper_t_k = 0.;
    double rcshaper_t_k_m1 = 0.;

    // Lambda for amplification
    auto amplification = [=](double t) { return A * (exp(-t / t_F_) - exp(-t / t_R_)) * (1. - exp(-t / t_S_)); };

    // Lambda for feedback
    auto feedback = [=](double U) { return Fb_ * (1. - exp(-U / Fb_D_)); };

    // Lambda for saturation
    auto saturation = [=](double U) { return U_sat_ * (2. / (1. + exp(-2. * U / U_sat_)) - 1.); };

    // Log some useful information
    LOG(DEBUG) << "Amplifying pulse with effective charge " << Units::display(charge, "e") << " arriving at "
               << Units::display(static_cast<double>(kmin) * timestep, "ns") << ", A = " << Units::display(A, "mV");

    // Set output to zero before pulse arrives
    std::fill_n(amplified_pulse_vec.begin(), kmin, 0.);

    // Calculate output starting relative to arrival
    for(size_t k = kmin; k < ntimepoints; ++k) {
        // Add voltage from RC shaper
        rcshaper_t_k = amplification(static_cast<double>(k - kmin) * timestep);
        U_out += rcshaper_t_k - rcshaper_t_k_m1;

        // Apply feedback
        U_out -= feedback(U_out) * timestep;

        // Store output
        amplified_pulse_vec.at(k) = U_out;
        rcshaper_t_k_m1 = rcshaper_t_k;
    }

    // Apply saturation
    std::transform(amplified_pulse_vec.begin(),
                   amplified_pulse_vec.end(),
                   amplified_pulse_vec.begin(),
                   [&saturation](auto& c) { return saturation(c); });

    return amplified_pulse_vec;
}
