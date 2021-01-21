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

    config_.setDefault<double>("integration_time", Units::get(500, "ns"));
    config_.setDefault<double>("threshold", Units::get(10e-3, "V"));
    config_.setDefault<bool>("ignore_polarity", false);

    config_.setDefault<double>("sigma_noise", Units::get(1e-4, "V"));

    config_.setDefault<bool>("output_pulsegraphs", false);
    config_.setDefault<bool>("output_plots", config_.get<bool>("output_pulsegraphs"));
    config_.setDefault<int>("output_plots_scale", Units::get(30, "ke"));
    config_.setDefault<int>("output_plots_bins", 100);

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

    // Time-of-Arrival
    if(config_.has("clock_bin_toa")) {
        store_toa_ = true;
        clockToA_ = config_.get<double>("clock_bin_toa");
    }
    // Time-over-Threshold
    if(config_.has("clock_bin_tot")) {
        store_tot_ = true;
        clockToT_ = config_.get<double>("clock_bin_tot");
    }

    sigmaNoise_ = config_.get<double>("sigma_noise");
    threshold_ = config_.get<double>("threshold");
    ignore_polarity_ = config.get<bool>("ignore_polarity");

    if(model_ == DigitizerType::SIMPLE) {
        tauF_ = config_.get<double>("feedback_time_constant");
        tauR_ = config_.get<double>("rise_time_constant");
        auto capacitance_feedback = config_.get<double>("feedback_capacitance");
        resistance_feedback_ = tauF_ / capacitance_feedback;
        LOG(DEBUG) << "Parameters: cf = " << Units::display(capacitance_feedback, {"C/V", "fC/mV"})
                   << ", rf = " << Units::display(resistance_feedback_, "V*s/C")
                   << ", tauF = " << Units::display(tauF_, {"ns", "us", "ms", "s"})
                   << ", tauR = " << Units::display(tauR_, {"ns", "us", "ms", "s"});
    } else if(model_ == DigitizerType::CSA) {
        auto ikrum = config_.get<double>("krummenacher_current");
        if(ikrum <= 0) {
            InvalidValueError(
                config_, "krummenacher_current", "The Krummenacher feedback current has to be positive definite.");
        }

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
        LOG(DEBUG) << "Parameters: rf = " << Units::display(resistance_feedback_, "V*s/C")
                   << ", capacitance_feedback = " << Units::display(capacitance_feedback, {"C/V", "fC/mV"})
                   << ", capacitance_detector = " << Units::display(capacitance_detector, {"C/V", "fC/mV"})
                   << ", capacitance_output = " << Units::display(capacitance_output, {"C/V", "fC/mV"})
                   << ", gm = " << Units::display(gm, "C/s/V")
                   << ", tauF = " << Units::display(tauF_, {"ns", "us", "ms", "s"})
                   << ", tauR = " << Units::display(tauR_, {"ns", "us", "ms", "s"})
                   << ", temperature = " << Units::display(config_.get<double>("temperature"), "K");
    }

    output_plots_ = config_.get<bool>("output_plots");
    output_pulsegraphs_ = config_.get<bool>("output_pulsegraphs");
}

void CSADigitizerModule::init() {

    // Check for sensible configuration of threshold:
    if(ignore_polarity_ && threshold_ < 0) {
        LOG(WARNING)
            << "Negative threshold configured but signal polarity is ignored, this might lead to unexpected results.";
    }

    if(output_plots_) {
        LOG(TRACE) << "Creating output plots";

        // Plot axis are in kilo electrons - convert from framework units!
        int maximum = static_cast<int>(Units::convert(config_.get<int>("output_plots_scale"), "ke"));
        auto nbins = config_.get<int>("output_plots_bins");

        // Create histograms if needed
        h_tot = new TH1D("signal",
                         (store_tot_ ? "Time-over-Threshold;time over threshold [clk];pixels" : "Signal;signal;pixels"),
                         nbins,
                         0,
                         (store_tot_ ? integration_time_ / clockToT_ : 1000));
        h_toa = new TH1D(
            "time",
            (store_toa_ ? "Time-of-Arrival;time of arrival [clk];pixels" : "Time-of-Arrival;time of arrival [ns];pixels"),
            nbins,
            0,
            (store_toa_ ? integration_time_ / clockToA_ : integration_time_));
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
    for(const auto& pixel_charge : pixel_message_->getData()) {
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

            if(output_plots_) {
                // Generate x-axis:
                std::vector<double> time(impulse_response_function_.size());
                // clang-format off
                std::generate(time.begin(), time.end(), [n = 0.0, timestep]() mutable {  auto now = n; n += timestep; return now; });
                // clang-format on

                auto* response_graph = new TGraph(
                    static_cast<int>(impulse_response_function_.size()), &time[0], &impulse_response_function_[0]);
                response_graph->GetXaxis()->SetTitle("t [ns]");
                response_graph->GetYaxis()->SetTitle("amp. response");
                response_graph->SetTitle("Amplifier response function");
                getROOTDirectory()->WriteTObject(response_graph, "response_function");
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

        if(output_pulsegraphs_) {
            // Fill a graph with the pulse:
            create_output_pulsegraphs(std::to_string(event_num),
                                      std::to_string(pixel_index.x()) + "-" + std::to_string(pixel_index.y()),
                                      "amp_pulse",
                                      "Amplifier signal without noise",
                                      timestep,
                                      amplified_pulse_vec);
        }

        // Apply noise to the amplified pulse
        std::normal_distribution<double> pulse_smearing(0, sigmaNoise_);
        LOG(TRACE) << "Adding electronics noise with sigma = " << Units::display(sigmaNoise_, {"mV", "V"});
        std::transform(amplified_pulse_vec.begin(),
                       amplified_pulse_vec.end(),
                       amplified_pulse_vec.begin(),
                       [&pulse_smearing, this](auto& c) { return c + (pulse_smearing(random_generator_)); });

        // Fill a graphs with the individual pixel pulses:
        if(output_pulsegraphs_) {
            create_output_pulsegraphs(std::to_string(event_num),
                                      std::to_string(pixel_index.x()) + "-" + std::to_string(pixel_index.y()),
                                      "amp_pulse_noise",
                                      "Amplifier signal with added noise",
                                      timestep,
                                      amplified_pulse_vec);
        }

        // Find threshold crossing - if any:
        auto arrival = get_toa(timestep, amplified_pulse_vec);
        if(!std::get<0>(arrival)) {
            LOG(DEBUG) << "Amplified signal never crossed threshold, continuing.";
            continue;
        }

        // Decide whether to store ToA or arrival time:
        auto time = (store_toa_ ? static_cast<double>(std::get<1>(arrival)) : std::get<2>(arrival));

        // Decide whether to store ToT or the pulse integral:
        auto charge = (store_tot_ ? static_cast<double>(get_tot(timestep, std::get<2>(arrival), amplified_pulse_vec))
                                  : std::accumulate(amplified_pulse_vec.begin(), amplified_pulse_vec.end(), 0.0));

        LOG(DEBUG) << "Pixel " << pixel_index << ": time "
                   << (store_toa_ ? std::to_string(static_cast<int>(time)) + "clk"
                                  : Units::display(time, {"ps", "ns", "us"}))
                   << ", signal "
                   << (store_tot_ ? std::to_string(static_cast<int>(charge)) + "clk"
                                  : Units::display(charge, {"V*s", "mV*s"}));

        // Fill histograms if requested
        if(output_plots_) {
            h_tot->Fill(charge);
            h_toa->Fill(time);
            h_pxq_vs_tot->Fill(inputcharge / 1e3, charge);
        }

        // Add the hit to the hitmap
        hits.emplace_back(pixel, time, pixel_charge.getGlobalTime() + time, charge, &pixel_charge);
    }

    // Output summary and update statistics
    LOG(INFO) << "Digitized " << hits.size() << " pixel hits";

    if(!hits.empty()) {
        // Create and dispatch hit message
        auto hits_message = std::make_shared<PixelHitMessage>(std::move(hits), getDetector());
        messenger_->dispatchMessage(this, hits_message);
    }
}

std::tuple<bool, unsigned int, double> CSADigitizerModule::get_toa(double timestep, const std::vector<double>& pulse) const {

    LOG(TRACE) << "Calculating time-of-arrival";
    bool threshold_crossed = false;
    unsigned int comparator_cycles = 0;
    double arrival_time = 0;

    // Lambda for threshold calculation:
    auto is_above_threshold = [](double bin, double threshold, bool ignore_polarity) {
        if(ignore_polarity) {
            return (std::fabs(bin) > std::fabs(threshold));
        } else {
            return (threshold > 0 ? bin > threshold : bin < threshold);
        }
    };

    // Find the point where the signal crosses the threshold, latch ToA
    while(arrival_time < integration_time_) {
        auto bin = pulse.at(static_cast<size_t>(std::floor(arrival_time / timestep)));
        if(is_above_threshold(bin, threshold_, ignore_polarity_)) {
            threshold_crossed = true;
            break;
        };
        comparator_cycles++;
        arrival_time += (store_toa_ ? clockToA_ : timestep);
    }
    return {threshold_crossed, comparator_cycles, arrival_time};
}

unsigned int CSADigitizerModule::get_tot(double timestep, double arrival_time, const std::vector<double>& pulse) const {

    LOG(TRACE) << "Calculating time-over-threshold, starting at " << Units::display(arrival_time, {"ps", "ns", "us"});
    unsigned int tot_clock_cycles = 0;

    // Lambda for threshold calculation:
    auto is_below_threshold = [](double bin, double threshold, bool ignore_polarity) {
        if(ignore_polarity) {
            return (std::fabs(bin) < std::fabs(threshold));
        } else {
            return (threshold > 0 ? bin < threshold : bin > threshold);
        }
    };

    // Start calculation from the next ToT clock cycle following the threshold crossing
    auto tot_time = clockToT_ * std::ceil(arrival_time / clockToT_);
    while(tot_time < integration_time_) {
        auto bin = pulse.at(static_cast<size_t>(std::floor(tot_time / timestep)));
        if(is_below_threshold(bin, threshold_, ignore_polarity_)) {
            break;
        }
        tot_clock_cycles++;
        tot_time += clockToT_;
    }
    return tot_clock_cycles;
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
    std::transform(
        plot_pulse_vec.begin(), plot_pulse_vec.end(), pulse_in_mV.begin(), [](auto& c) { return Units::convert(c, "mV"); });

    std::string name = s_name + "_ev" + s_event_num + "_px" + s_pixel_index;
    auto* csa_pulse_graph = new TGraph(static_cast<int>(pulse_in_mV.size()), &amptime[0], &pulse_in_mV[0]);
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
