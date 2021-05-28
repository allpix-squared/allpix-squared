/**
 * @file
 * @brief Implementation of MuPix digitization module
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "MuPixDigitizerModule.hpp"

#include "core/utils/distributions.h"
#include "core/utils/unit.h"
#include "tools/ROOT.h"

#include <TF1.h>
#include <TFile.h>
#include <TGraph.h>
#include <TH1D.h>
#include <TH1F.h>
#include <TProfile.h>

#include "objects/PixelHit.hpp"

#include "Models/MuPix10.hpp"
#include "Models/MuPix10Double.hpp"
#include "Models/MuPix10Ramp.hpp"

using namespace allpix;

MuPixDigitizerModule::MuPixDigitizerModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector)
    : Module(config, std::move(detector)), messenger_(messenger) {

    // Require PixelCharge message for single detector
    messenger_->bindSingle<PixelChargeMessage>(this, MsgFlags::REQUIRED);

    // Read model
    auto model = config_.get<std::string>("model");
    std::transform(model.begin(), model.end(), model.begin(), ::tolower);
    if(model == "mupix10") {
        model_ = std::make_unique<mupix::MuPix10>(config);
    } else if(model == "mupix10double") {
        model_ = std::make_unique<mupix::MuPix10Double>(config);
    } else if(model == "mupix10ramp") {
        model_ = std::make_unique<mupix::MuPix10Ramp>(config);
    } else {
        throw InvalidValueError(
            config_, "model", "Invalid model, only 'mupix10', 'mupix10double' and 'mupix10ramp' are supported.");
    }

    // Set defaults for common config variables
    config_.setDefault<double>("sigma_noise", Units::get(1e-3, "V"));
    config_.setDefault<bool>("output_pulsegraphs", false);
    config_.setDefault<bool>("output_plots", config_.get<bool>("output_pulsegraphs"));

    // Get common config variables
    sigmaNoise_ = config_.get<double>("sigma_noise");
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

void MuPixDigitizerModule::initialize() {
    if(output_plots_) {
        LOG(TRACE) << "Creating output plots";

        auto nbins_ts1 = static_cast<int>(std::ceil(model_->get_integration_time() / model_->get_ts1_clock()));
        auto nbins_ts2 = static_cast<int>(std::ceil(model_->get_ts2_integration_time() / model_->get_ts2_clock()));
        auto nbins_tot = static_cast<int>(std::ceil(model_->get_ts2_integration_time() / model_->get_ts1_clock()));

        // Create histograms if needed
        h_ts1 = CreateHistogram<TH1D>("ts1", "TS1;TS1 [clk];pixels", nbins_ts1, 0, nbins_ts1);
        h_ts2 = CreateHistogram<TH1D>("ts2", "TS2;TS2 [clk];pixels", nbins_ts2, 0, nbins_ts2);
        h_tot = CreateHistogram<TH1F>("tot", "ToT;TS2 - TS1 [ns];pixels", nbins_tot, 0, model_->get_ts2_integration_time());
    }
}

void MuPixDigitizerModule::run(Event* event) {
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

        auto pulse_vec = pulse.getPulse(); // the vector of the charges
        auto timestep = pulse.getBinning();

        LOG(TRACE) << "Preparing pulse for pixel " << pixel_index << ", " << pulse_vec.size() << " bins of "
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

        // Fill a graphs with the individual pixel pulses
        if(output_pulsegraphs_) {
            create_output_pulsegraphs(std::to_string(event->number),
                                      std::to_string(pixel_index.x()) + "-" + std::to_string(pixel_index.y()),
                                      "amp_pulse_noise",
                                      "Amplifier signal with added noise",
                                      timestep,
                                      amplified_pulse_vec);
        }

        // Find threshold crossing - if any
        auto ts1_result = model_->get_ts1(timestep, amplified_pulse_vec);
        if(!std::get<0>(ts1_result)) {
            LOG(DEBUG) << "Amplified signal never crossed threshold, continuing.";
            continue;
        }

        auto ts1 = std::get<1>(ts1_result);

        // Find second time stamp
        auto ts2 = model_->get_ts2(ts1, timestep, amplified_pulse_vec);

        LOG(DEBUG) << "Pixel " << pixel_index << ": TS1 " << std::to_string(ts1) + "clk"
                   << ", TS2 " << std::to_string(ts2) + "clk";

        // Fill histograms if requested
        if(output_plots_) {
            h_ts1->Fill(ts1);
            h_ts2->Fill(ts2);
            h_tot->Fill(ts2 * model_->get_ts2_clock() - ts1 * model_->get_ts1_clock());
        }

        // Add the hit to the hitmap
        hits.emplace_back(pixel,
                          static_cast<double>(ts1),
                          pixel_charge.getGlobalTime() + ts1 * model_->get_ts1_clock(),
                          static_cast<double>(ts2),
                          &pixel_charge);
    }

    // Output summary and update statistics
    LOG(INFO) << "Digitized " << hits.size() << " pixel hits";

    if(!hits.empty()) {
        // Create and dispatch hit message
        auto hits_message = std::make_shared<PixelHitMessage>(std::move(hits), getDetector());
        messenger_->dispatchMessage(this, hits_message, event);
    }
}

void MuPixDigitizerModule::create_output_pulsegraphs(const std::string& s_event_num,
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

void MuPixDigitizerModule::finalize() {
    if(output_plots_) {
        // Write histograms
        LOG(TRACE) << "Writing output plots to file";
        h_ts1->Write();
        h_ts2->Write();
        h_tot->Write();
    }
}
