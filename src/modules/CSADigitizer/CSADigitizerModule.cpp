/**
 * @file
 * @brief Implementation of charge sensitive amplifier digitization module
 * @copyright Copyright (c) 2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "CSADigitizerModule.hpp"

#include "core/utils/unit.h"
#include "tools/ROOT.h"

#include <TFile.h>
#include <TGraph.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>

#include "objects/PixelHit.hpp"

using namespace allpix;

CSADigitizerModule::CSADigitizerModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector)
    : Module(config, std::move(detector)), messenger_(messenger), pixel_message_(nullptr) {
    // Enable parallelization of this module if multithreading is enabled
    enable_parallelization();

    // Require PixelCharge message for single detector
    messenger_->bindSingle(this, &CSADigitizerModule::pixel_message_, MsgFlags::REQUIRED);

    // Seed the random generator with the global seed
    random_generator_.seed(getRandomSeed());

    // Read model
    auto model = config_.get<std::string>("model");
    std::transform(model.begin(), model.end(), model.begin(), ::tolower);
    if(model == "simple") {
        model_ = DigitizerType::SIMPLE;
    } else if(model == "csa") {
        model_ = DigitizerType::CSA;
    } else {
        throw InvalidValueError(config_, "model", "Invalid model, only 'simple' and 'csa' are supported.");
    }

    // Set defaults for config variables
    config_.setDefault<double>("feedback_capacitance", Units::get(5e-15, "C/V"));

    config_.setDefault<double>("integration_time", Units::get(0.5e-6, "s"));
    config_.setDefault<double>("threshold", Units::get(10e-3, "V"));
    config_.setDefault<double>("sigma_noise", Units::get(1e-4, "V"));
    config_.setDefault<double>("clock_bin_toa", Units::get(1.5625, "ns"));
    config_.setDefault<double>("clock_bin_tot", Units::get(25.0, "ns"));

    config_.setDefault<bool>("output_pulsegraphs", false);
    config_.setDefault<bool>("output_plots", config_.get<bool>("output_pulsegraphs"));
    config_.setDefault<int>("output_plots_scale", Units::get(30, "ke"));
    config_.setDefault<int>("output_plots_bins", 100);

    config_.setDefault<bool>("store_tot", true);

    if(model_ == DigitizerType::SIMPLE) {
        // defaults for the "simple" parametrisation
        config_.setDefault<double>("rise_time_constant", Units::get(1e-9, "s"));
        config_.setDefault<double>("feedback_time_constant", Units::get(10e-9, "s")); // R_f * C_f
    } else if(model_ == DigitizerType::CSA) {
        // and for the "advanced" csa
        config_.setDefault<double>("krummenacher_current", Units::get(20e-9, "C/s"));
        config_.setDefault<double>("detector_capacitance", Units::get(100e-15, "C/V"));
        config_.setDefault<double>("amp_output_capacitance", Units::get(20e-15, "C/V"));
        config_.setDefault<double>("transconductance", Units::get(50e-6, "C/s/V"));
        config_.setDefault<double>("temperature", 293.15);
    }

    // Copy some variables from configuration to avoid lookups:
    integration_time_ = config_.get<double>("integration_time");
    clockToA_ = config_.get<double>("clock_bin_toa");
    clockToT_ = config_.get<double>("clock_bin_tot");
    sigmaNoise_ = config_.get<double>("sigma_noise");
    threshold_ = config_.get<double>("threshold");

    if(model_ == DigitizerType::SIMPLE) {
        tauF_ = config_.get<double>("feedback_time_constant");
        tauR_ = config_.get<double>("rise_time_constant");
        auto capacitance_feedback = config_.get<double>("feedback_capacitance");
        resistance_feedback_ = tauF_ / capacitance_feedback;
        LOG(DEBUG) << "Parameters: cf " << Units::display(capacitance_feedback, "C/V") << ", rf "
                   << Units::display(resistance_feedback_, "V*s/C") << ", tauF_ " << Units::display(tauF_, "s") << ", tauR_ "
                   << Units::display(tauR_, "s");
    } else if(model_ == DigitizerType::CSA) {
        auto ikrum = config_.get<double>("krummenacher_current");
        auto capacitance_detector = config_.get<double>("detector_capacitance");
        auto capacitance_feedback = config_.get<double>("feedback_capacitance");
        auto capacitance_output = config_.get<double>("amp_output_capacitance");
        auto gm = config_.get<double>("transconductance");
        auto boltzmann_kT = Units::get(8.6173e-5, "eV/K") * config_.get<double>("temperature");

        // helper variables: transconductance and resistance in the feedback loop
        // weak inversion: gf = I/(n V_t) (e.g. Binkley "Tradeoff and Optimisation in Analog CMOS design")
        // n is the weak inversion slope factor (degradation of exponential MOS drain current compared to bipolar transistor
        // collector current) n_wi typically 1.5, for circuit described in  Kleczek 2016 JINST11 C12001: I->I_krumm/2
        auto transconductance_feedback = ikrum / (2.0 * 1.5 * boltzmann_kT);
        resistance_feedback_ = 2. / transconductance_feedback; // feedback resistor
        tauF_ = resistance_feedback_ * capacitance_feedback;
        tauR_ = (capacitance_detector * capacitance_output) / (gm * capacitance_feedback);
        LOG(DEBUG) << "Parameters: rf " << Units::display(resistance_feedback_, "V*s/C") << ", capacitance_feedback "
                   << Units::display(capacitance_feedback, "C/V") << ", capacitance_detector "
                   << Units::display(capacitance_detector, "C/V") << ", capacitance_output "
                   << Units::display(capacitance_output, "C/V") << ", gm " << Units::display(gm, "C/s/V") << ", tauF_ "
                   << Units::display(tauF_, "s") << ", tauR_ " << Units::display(tauR_, "s") << ", temperature "
                   << config_.get<double>("temperature") << "K";
    }
    store_tot_ = config_.get<bool>("store_tot");
    output_plots_ = config_.get<bool>("output_plots");
    output_pulsegraphs_ = config_.get<bool>("output_pulsegraphs");
}

void CSADigitizerModule::init() {

    if(output_plots_) {
        LOG(TRACE) << "Creating output plots";

        // Plot axis are in kilo electrons - convert from framework units!
        int maximum = static_cast<int>(Units::convert(config_.get<int>("output_plots_scale"), "ke"));
        auto nbins = config_.get<int>("output_plots_bins");

        // Create histograms if needed
        h_tot = new TH1D("tot", "time over threshold;time over threshold [ns];pixels", nbins, 0, integration_time_);
        h_toa = new TH1D("toa", "time of arrival;time of arrival [ns];pixels", nbins, 0, integration_time_);
        h_pxq_vs_tot = new TH2D("pxqvstot",
                                "ToT vs raw pixel charge;pixel charge [ke];ToT [ns]",
                                nbins,
                                0,
                                maximum,
                                nbins,
                                0,
                                integration_time_);
    }
}

void CSADigitizerModule::run(unsigned int event_num) {
    // Loop through all pixels with charges
    std::vector<PixelHit> hits;
    for(auto& pixel_charge : pixel_message_->getData()) {
        auto pixel = pixel_charge.getPixel();
        auto pixel_index = pixel.getIndex();
        auto inputcharge = static_cast<double>(pixel_charge.getCharge());

        LOG(DEBUG) << "Received pixel " << pixel_index << ", charge " << Units::display(inputcharge, "e");

        const auto& pulse = pixel_charge.getPulse(); // the pulse containing charges and times
        auto pulse_vec = pulse.getPulse();           // the vector of the charges
        auto timestep = pulse.getBinning();
        auto ntimepoints = static_cast<size_t>(ceil(integration_time_ / timestep));

        std::call_once(first_event_flag_, [&]() {
            // initialize impulse response function - assume all time bins are equal
            impulse_response_function_.reserve(ntimepoints);
            auto calculate_impulse_response = [&](double x) {
                return (resistance_feedback_ * (exp(-x / tauF_) - exp(-x / tauR_)) / (tauF_ - tauR_));
            };
            for(size_t itimepoint = 0; itimepoint < ntimepoints; ++itimepoint) {
                impulse_response_function_.push_back(calculate_impulse_response(timestep * static_cast<double>(itimepoint)));
            }
            LOG(INFO) << "Initialized impulse response with timestep " << Units::display(timestep, {"ps", "ns", "us"})
                      << " and integration time " << Units::display(integration_time_, {"ns", "us", "ms"})
                      << ", samples: " << ntimepoints;
        });

        std::vector<double> amplified_pulse_vec(ntimepoints);
        auto input_length = pulse_vec.size();
        LOG(TRACE) << "Preparing pulse for pixel " << pixel_index << ", " << pulse_vec.size() << " bins of "
                   << Units::display(timestep, {"ps", "ns"}) << ", total charge: " << Units::display(pulse.getCharge(), "e");
        // convolution of the pulse (size input_length) with the impulse response (size ntimepoints)
        for(size_t k = 0; k < ntimepoints; ++k) {
            double outsum{};
            // convolution: multiply pulse_vec.at(k - i) * impulse_response_function_.at(i), when (k - i) < input_length
            // -> no point to start i at 0, start from jmin:
            size_t jmin = (k >= input_length - 1) ? k - (input_length - 1) : 0;
            for(size_t i = jmin; i <= k; ++i) {
                if((k - i) < input_length) {
                    outsum += pulse_vec.at(k - i) * impulse_response_function_.at(i);
                }
            }
            amplified_pulse_vec.at(k) = outsum;
        }

        // apply noise on the amplified pulse
        std::normal_distribution<double> pulse_smearing(0, sigmaNoise_);
        std::vector<double> amplified_pulse_with_noise(amplified_pulse_vec.size());
        std::transform(amplified_pulse_vec.begin(),
                       amplified_pulse_vec.end(),
                       amplified_pulse_with_noise.begin(),
                       [&pulse_smearing, this](auto& c) { return c + (pulse_smearing(random_generator_)); });

        // TOA and TOT logic
        auto compare_result = compare_with_threshold(timestep, amplified_pulse_with_noise);
        LOG(DEBUG) << "Pixel " << pixel_index << ": ToA = " << Units::display(compare_result.first, {"ps", "ns", "us"})
                   << ", ToT = " << Units::display(compare_result.second, {"ps", "ns", "us"});

        // Fill histograms if requested
        if(output_plots_) {
            h_tot->Fill(compare_result.second);
            h_toa->Fill(compare_result.first);
            h_pxq_vs_tot->Fill(inputcharge / 1e3, compare_result.second);
        }

        // Fill a graphs with the individual pixel pulses:
        if(output_pulsegraphs_) {
            create_output_pulsegraphs(std::to_string(event_num),
                                      std::to_string(pixel_index.x()) + "-" + std::to_string(pixel_index.y()),
                                      "csa_pulse_before_noise",
                                      "Amplifier signal without noise",
                                      timestep,
                                      amplified_pulse_vec);

            create_output_pulsegraphs(std::to_string(event_num),
                                      std::to_string(pixel_index.x()) + "-" + std::to_string(pixel_index.y()),
                                      "csa_pulse_with_noise",
                                      "Amplifier signal with added noise",
                                      timestep,
                                      amplified_pulse_with_noise);
        }

        // Add the hit to the hitmap
        if(store_tot_) {
            hits.emplace_back(pixel, compare_result.first, compare_result.second, &pixel_charge);
        } else {
            // calculate pulse integral with noise
            auto amplified_pulse_integral =
                std::accumulate(amplified_pulse_with_noise.begin(), amplified_pulse_with_noise.end(), 0.0);
            hits.emplace_back(pixel, compare_result.first, amplified_pulse_integral, &pixel_charge);
        }
    }

    // Output summary and update statistics
    LOG(INFO) << "Digitized " << hits.size() << " pixel hits";

    if(!hits.empty()) {
        // Create and dispatch hit message
        auto hits_message = std::make_shared<PixelHitMessage>(std::move(hits), getDetector());
        messenger_->dispatchMessage(this, hits_message);
    }
}

std::pair<double, double> CSADigitizerModule::compare_with_threshold(double timestep,
                                                                     const std::vector<double>& amplified_pulse_with_noise) {

    bool is_over_threshold = false;
    double toa{}, tot{};
    double jtoa{}, jtot{};
    // first find the point where the signal crosses the threshold, latch toa
    while(jtoa < integration_time_) {
        if(amplified_pulse_with_noise.at(static_cast<size_t>(floor(jtoa / timestep))) > threshold_) {
            is_over_threshold = true;
            toa = jtoa;
            break;
        };
        jtoa += clockToA_;
    }

    // only look for ToT if the threshold was crossed in the ToA loop
    // start from the next tot clock cycle following toa
    jtot = clockToT_ * (ceil(toa / clockToT_));
    while(is_over_threshold && jtot < integration_time_) {
        if(amplified_pulse_with_noise.at(static_cast<size_t>(floor(jtot / timestep))) > threshold_) {
            tot += clockToT_;
        } else {
            is_over_threshold = false;
        }
        jtot += clockToT_;
    }

    return {toa, tot};
}

void CSADigitizerModule::create_output_pulsegraphs(const std::string& s_event_num,
                                                   const std::string& s_pixel_index,
                                                   const std::string& s_name,
                                                   const std::string& s_title,
                                                   double timestep,
                                                   const std::vector<double>& plot_pulse_vec) {

    // Generate x-axis:
    std::vector<double> amptime(plot_pulse_vec.size());
    // clang-format off
    std::generate(amptime.begin(), amptime.end(), [n = 0.0, timestep]() mutable {  auto now = n; n += timestep; return now; });
    // clang-format on

    // scale the y-axis values to be in mV instead of MV
    std::vector<double> pulse_in_mV(plot_pulse_vec.size());
    double scale = 1e9;
    std::transform(
        plot_pulse_vec.begin(), plot_pulse_vec.end(), pulse_in_mV.begin(), [&scale](auto& c) { return c * scale; });

    std::string name = s_name + "_ev" + s_event_num + "_px" + s_pixel_index;
    auto csa_pulse_graph = new TGraph(static_cast<int>(pulse_in_mV.size()), &amptime[0], &pulse_in_mV[0]);
    csa_pulse_graph->GetXaxis()->SetTitle("t [ns]");
    csa_pulse_graph->GetYaxis()->SetTitle("CSA output [mV]");
    csa_pulse_graph->SetTitle((s_title + " in pixel (" + s_pixel_index + ")").c_str());
    getROOTDirectory()->WriteTObject(csa_pulse_graph, name.c_str());
}

void CSADigitizerModule::finalize() {
    if(output_plots_) {
        // Write histograms
        LOG(TRACE) << "Writing output plots to file";
        h_tot->Write();
        h_toa->Write();
        h_pxq_vs_tot->Write();
    }
}
