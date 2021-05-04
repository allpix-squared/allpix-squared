/**
 * @file
 * @brief Implementation of the Krummenacher Current CSADigitizerModel class
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "KrummenacherCurrentModel.hpp"

#include "tools/ROOT.h"

using namespace allpix::csa;

void KrummenacherCurrentModel::configure(Configuration& config) {
    // Don't configure simple model, as we use other parameters for the same impulse response function
    CSADigitizerModel::configure(config);

    config.setDefault<double>("impulse_response_timestep", Units::get(0.01, "ns")); // FIXME: get from PulseTranfer (?)
    config.setDefault<double>("feedback_capacitance", Units::get(5e-15, "C/V"));
    config.setDefault<double>("krummenacher_current", Units::get(20e-9, "C/s"));
    config.setDefault<double>("detector_capacitance", Units::get(100e-15, "C/V"));
    config.setDefault<double>("amp_output_capacitance", Units::get(20e-15, "C/V"));
    config.setDefault<double>("transconductance", Units::get(50e-6, "C/s/V"));
    config.setDefault<double>("temperature", 293.15);

    // Get parameters to determine impulse response function parameters
    timestep_ = config.get<double>("impulse_response_timestep");
    auto ikrum = config.get<double>("krummenacher_current");
    if(ikrum <= 0) {
        InvalidValueError(config, "krummenacher_current", "The Krummenacher feedback current has to be positive definite.");
    }

    auto capacitance_detector = config.get<double>("detector_capacitance");
    auto capacitance_feedback = config.get<double>("feedback_capacitance");
    auto capacitance_output = config.get<double>("amp_output_capacitance");
    auto gm = config.get<double>("transconductance");
    auto temperature = config.get<double>("temperature");
    auto boltzmann_kT = Units::get(8.6173e-5, "eV/K") * temperature;

    // helper variables: transconductance and resistance in the feedback loop
    // weak inversion: gf = I/(n V_t) (e.g. Binkley "Tradeoff and Optimisation in Analog CMOS design")
    // n is the weak inversion slope factor (degradation of exponential MOS drain current compared to bipolar transistor
    // collector current) n_wi typically 1.5, for circuit described in  Kleczek 2016 JINST11 C12001: I->I_krumm/2
    auto transconductance_feedback = ikrum / (2.0 * 1.5 * boltzmann_kT);
    resistance_feedback_ = 2. / transconductance_feedback; // feedback resistor
    tauF_ = resistance_feedback_ * capacitance_feedback;
    tauR_ = (capacitance_detector * capacitance_output) / (gm * capacitance_feedback);

    LOG(DEBUG) << "Parameters: rf = " << Units::display(resistance_feedback_, "V*s/C")
               << ", capacitance_feedback = " << Units::display(capacitance_feedback, {"C/V", "fC/mV"})
               << ", capacitance_detector = " << Units::display(capacitance_detector, {"C/V", "fC/mV"})
               << ", capacitance_output = " << Units::display(capacitance_output, {"C/V", "fC/mV"})
               << ", gm = " << Units::display(gm, "C/s/V") << ", tauF = " << Units::display(tauF_, {"ns", "us", "ms", "s"})
               << ", tauR = " << Units::display(tauR_, {"ns", "us", "ms", "s"})
               << ", temperature = " << Units::display(temperature, "K");

    precalculate_impulse_response();
}
