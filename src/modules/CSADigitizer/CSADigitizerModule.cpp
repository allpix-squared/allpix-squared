/**
 * @file
 * @brief Implementation of charge sensitive amplifier digitization module
 * @copyright Copyright (c) 2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "CSADigitizerModule.hpp"

#include "core/utils/distributions.h"
#include "core/utils/unit.h"
#include "tools/ROOT.h"

#include <TF1.h>
#include <TFile.h>
#include <TGraph.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>

#include "objects/PixelHit.hpp"

#include "CSADigitizerModel.hpp"
#include "Models/KrummenacherCurrentModel.hpp"
#include "Models/MuPix10.hpp"
#include "Models/SimpleModel.hpp"

using namespace allpix;

CSADigitizerModule::CSADigitizerModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector)
    : Module(config, std::move(detector)), messenger_(messenger) {

    // Require PixelCharge message for single detector
    messenger_->bindSingle<PixelChargeMessage>(this, MsgFlags::REQUIRED);

    // Read model
    auto model = config_.get<std::string>("model");
    std::transform(model.begin(), model.end(), model.begin(), ::tolower);
    if(model == "simple") {
        model_ = std::make_unique<csa::SimpleModel>();
    } else if(model == "krummenacher") {
        model_ = std::make_unique<csa::KrummenacherCurrentModel>();
    } else if(model == "mupix10") {
        model_ = std::make_unique<csa::MuPix10>();
    } else {
        throw InvalidValueError(
            config_, "model", "Invalid model, only 'simple', 'krummenacher' and 'mupix10' are supported.");
    }

    // Set defaults for config variables
    config_.setDefault<double>("integration_time", Units::get(500, "ns"));
    config_.setDefault<double>("threshold", Units::get(10e-3, "V"));
    config_.setDefault<bool>("ignore_polarity", false);

    config_.setDefault<double>("sigma_noise", Units::get(1e-4, "V"));

    config_.setDefault<bool>("output_pulsegraphs", false);
    config_.setDefault<bool>("output_plots", config_.get<bool>("output_pulsegraphs"));
    config_.setDefault<int>("output_plots_scale", Units::get(30, "ke"));
    config_.setDefault<int>("output_plots_bins", 100);

    // Configure model
    model_->configure(config);

    // Copy some variables from configuration to avoid lookups:
    integration_time_ = config_.get<double>("integration_time");

    // Time-of-Arrival
    if(config_.has("clock_bin_ts1")) {
        store_ts1_ = true;
        clockTS1_ = config_.get<double>("clock_bin_ts1");
    }
    // Time-over-Threshold
    if(config_.has("clock_bin_ts2")) {
        store_ts2_ = true;
        clockTS2_ = config_.get<double>("clock_bin_ts2");

        // Whether to store ToT or TS2
        config_.setDefault<bool>("signal_is_ts2", false);
        signal_is_ts2_ = config_.get<bool>("signal_is_ts2");
    }

    sigmaNoise_ = config_.get<double>("sigma_noise");
    threshold_ = config_.get<double>("threshold");
    ignore_polarity_ = config.get<bool>("ignore_polarity");

    output_plots_ = config_.get<bool>("output_plots");
    output_pulsegraphs_ = config_.get<bool>("output_pulsegraphs");

    // Enable parallelization of this module if multithreading is enabled and no per-event output plots are requested:
    // FIXME: Review if this is really the case or we can still use multithreading
    if(!output_pulsegraphs_) {
        enable_parallelization();
    } else {
        LOG(WARNING) << "Per-event pulse graphs requested, disabling parallel event processing";
    }
}

void CSADigitizerModule::initialize() {
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
        h_tot = CreateHistogram<TH1D>(
            "signal",
            (store_ts2_ ? "Time-over-Threshold;time over threshold [clk];pixels" : "Signal;signal;pixels"),
            (store_ts2_ ? static_cast<int>(integration_time_ / clockTS2_) : nbins),
            0,
            (store_ts2_ ? integration_time_ / clockTS2_ : 1000));
        h_toa = CreateHistogram<TH1D>(
            "time",
            (store_ts1_ ? "Time-of-Arrival;time of arrival [clk];pixels" : "Time-of-Arrival;time of arrival [ns];pixels"),
            (store_ts1_ ? static_cast<int>(integration_time_ / clockTS1_) : nbins),
            0,
            (store_ts1_ ? integration_time_ / clockTS1_ : integration_time_));
        h_pxq_vs_tot = CreateHistogram<TH2D>("pxqvstot",
                                             "ToT vs raw pixel charge;pixel charge [ke];ToT [ns]",
                                             nbins,
                                             0,
                                             maximum,
                                             nbins,
                                             0,
                                             integration_time_);
    }

    // This doesn't work as before, since the amplification can now be charge dependent
    // My idea would be to call this on init and do it for either 1e, 1C or some other
    // "sane" amount of charge (e.g. ~1ke). I do prefer the first one though.
    /*if(output_plots_) {
                // Generate x-axis:
                std::vector<double> time(impulse_response_function_.size());
                // clang-format off
                std::generate(time.begin(), time.end(), [n = 0.0, timestep]() mutable {  auto now = n; n += timestep; return
    now; });
                // clang-format on

                auto* response_graph = new TGraph(
                    static_cast<int>(impulse_response_function_.size()), &time[0], &impulse_response_function_[0]);
                response_graph->GetXaxis()->SetTitle("t [ns]");
                response_graph->GetYaxis()->SetTitle("amp. response");
                response_graph->SetTitle("Amplifier response function");
                getROOTDirectory()->WriteTObject(response_graph, "response_function");
    }*/
}

void CSADigitizerModule::run(Event* event) {
    auto pixel_message = messenger_->fetchMessage<PixelChargeMessage>(this, event);

    // Loop through all pixels with charges
    std::vector<PixelHit> hits;
    for(const auto& pixel_charge : pixel_message->getData()) {
        auto pixel = pixel_charge.getPixel();
        auto pixel_index = pixel.getIndex();
        auto inputcharge = static_cast<double>(pixel_charge.getCharge());

        LOG(DEBUG) << "Received pixel " << pixel_index << ", charge " << Units::display(inputcharge, "e");

        const auto& pulse = pixel_charge.getPulse(); // the pulse containing charges and times

        if(!pulse.isInitialized()) {
            throw ModuleError("No pulse information available.");
        }

        auto timestep = pulse.getBinning();

        LOG(DEBUG) << "Preparing pulse for pixel " << pixel_index << ", " << pulse.getPulse().size() << " bins of "
                   << Units::display(timestep, {"ps", "ns"}) << ", total charge: " << Units::display(pulse.getCharge(), "e");

        auto amplified_pulse_vec = model_->amplify_pulse(pulse);

        if(output_pulsegraphs_) {
            // Fill a graph with the pulse:
            create_output_pulsegraphs(std::to_string(event->number),
                                      std::to_string(pixel_index.x()) + "-" + std::to_string(pixel_index.y()),
                                      "amp_pulse",
                                      "Amplifier signal without noise",
                                      timestep,
                                      amplified_pulse_vec);
        }

        // Apply noise to the amplified pulse
        allpix::normal_distribution<double> pulse_smearing(0, sigmaNoise_);
        LOG(TRACE) << "Adding electronics noise with sigma = " << Units::display(sigmaNoise_, {"mV", "V"});
        std::transform(amplified_pulse_vec.begin(),
                       amplified_pulse_vec.end(),
                       amplified_pulse_vec.begin(),
                       [&pulse_smearing, &event](auto& c) { return c + (pulse_smearing(event->getRandomEngine())); });

        // Fill a graphs with the individual pixel pulses:
        if(output_pulsegraphs_) {
            create_output_pulsegraphs(std::to_string(event->number),
                                      std::to_string(pixel_index.x()) + "-" + std::to_string(pixel_index.y()),
                                      "amp_pulse_noise",
                                      "Amplifier signal with added noise",
                                      timestep,
                                      amplified_pulse_vec);
        }

        // Find threshold crossing - if any:
        bool threshold_crossed{};
        union {
            unsigned int ts1;
            double arrival_time;
        };
        if(store_ts1_) {
            auto result = model_->get_ts1(timestep, amplified_pulse_vec);
            threshold_crossed = std::get<0>(result);
            ts1 = std::get<1>(result);
        } else {
            auto result = model_->get_arrival(timestep, amplified_pulse_vec);
            threshold_crossed = std::get<0>(result);
            arrival_time = std::get<1>(result);
        }

        if(!threshold_crossed) {
            LOG(DEBUG) << "Amplified signal never crossed threshold, continuing.";
            continue;
        }

        // Store TS2 or Pulse Integral
        union {
            unsigned int ts2;
            double charge;
        };
        if(store_ts2_) {
            ts2 = model_->get_ts2(ts1, timestep, amplified_pulse_vec);
        } else {
            charge = model_->get_pulse_integral(amplified_pulse_vec);
        }

        // Convert to local_time and signal
        double local_time{}, global_time{pixel_charge.getGlobalTime()}, tot{}, signal{};
        if(store_ts1_) {
            local_time = ts1;
            global_time += ts1 * clockTS1_;
        } else {
            local_time = arrival_time;
            global_time += arrival_time;
        }
        if(store_ts2_) {
            tot = static_cast<double>(ts2) - ceil(ts1 * clockTS1_ / clockTS2_);
            signal = (signal_is_ts2_ ? static_cast<double>(ts2) : tot);
        } else {
            tot = charge;
            signal = charge;
        }

        LOG(DEBUG) << "Pixel " << pixel_index << ": time "
                   << (store_ts1_ ? std::to_string(static_cast<int>(local_time)) + "clk"
                                  : Units::display(local_time, {"ps", "ns", "us"}))
                   << ", signal "
                   << (store_ts2_ ? std::to_string(static_cast<int>(signal)) + "clk"
                                  : Units::display(signal, {"V*s", "mV*s"}));

        // Fill histograms if requested
        if(output_plots_) {
            h_tot->Fill(tot);
            h_toa->Fill(local_time);
            h_pxq_vs_tot->Fill(inputcharge / 1e3, tot);
        }

        // Add the hit to the hitmap
        hits.emplace_back(pixel, local_time, global_time, signal, &pixel_charge);
    }

    // Output summary and update statistics
    LOG(INFO) << "Digitized " << hits.size() << " pixel hits";

    if(!hits.empty()) {
        // Create and dispatch hit message
        auto hits_message = std::make_shared<PixelHitMessage>(std::move(hits), getDetector());
        messenger_->dispatchMessage(this, hits_message, event);
    }
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
