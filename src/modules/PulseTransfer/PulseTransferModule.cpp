/**
 * @file
 * @brief Implementation of pulse transfer module
 * @copyright Copyright (c) 2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "PulseTransferModule.hpp"
#include "core/module/Event.hpp"
#include "core/utils/log.h"
#include "objects/PixelCharge.hpp"

#include <string>
#include <utility>

#include <TAxis.h>
#include <TGraph.h>

using namespace allpix;

PulseTransferModule::PulseTransferModule(Configuration& config,
                                         Messenger* messenger,
                                         const std::shared_ptr<Detector>& detector)
    : Module(config, detector), detector_(detector) {

    config_.setDefault<bool>("output_pulsegraphs", false);
    config_.setDefault<bool>("output_plots", config_.get<bool>("output_pulsegraphs"));
    config_.setDefault<int>("output_plots_scale", Units::get(30, "ke"));
    config_.setDefault<int>("output_plots_bins", 100);

    output_plots_ = config_.get<bool>("output_plots");
    output_pulsegraphs_ = config_.get<bool>("output_pulsegraphs");

    messenger->bindSingle<PulseTransferModule, PropagatedChargeMessage, MsgFlags::REQUIRED>(this);
}

void PulseTransferModule::init(std::mt19937_64&) {

    if(output_plots_) {
        LOG(TRACE) << "Creating output plots";

        // Plot axis are in kilo electrons - convert from framework units!
        int maximum = static_cast<int>(Units::convert(config_.get<int>("output_plots_scale"), "ke"));
        auto nbins = config_.get<int>("output_plots_bins");

        // Create histograms if needed
        h_total_induced_charge_ =
            new TH1D("inducedcharge", "total induced charge;induced charge [ke];events", nbins, 0, maximum);
        h_induced_pixel_charge_ =
            new TH1D("pixelcharge", "induced charge per pixel;induced pixel charge [ke];pixels", nbins, 0, maximum);
    }
}

void PulseTransferModule::run(Event* event) {
    auto propagated_message = event->fetchMessage<PropagatedChargeMessage>();

    // Create map for all pixels: pulse and propagated charges
    std::map<Pixel::Index, Pulse> pixel_pulse_map;
    std::map<Pixel::Index, std::vector<const PropagatedCharge*>, pixel_cmp> pixel_charge_map;

    LOG(DEBUG) << "Received " << propagated_message->getData().size() << " propagated charge objects.";
    for(auto& propagated_charge : propagated_message->getData()) {
        for(auto& pulse : propagated_charge.getPulses()) {
            auto pixel_index = pulse.first;

            // Accumulate all pulses from input message data:
            pixel_pulse_map[pixel_index] += pulse.second;

            auto px = pixel_charge_map[pixel_index];
            // For each pulse, store the corresponding propagated charges to preserve history:
            if(std::find(px.begin(), px.end(), &propagated_charge) == px.end()) {
                pixel_charge_map[pixel_index].emplace_back(&propagated_charge);
            }
        }
    }

    // Create vector of pixel pulses to return for this detector
    std::vector<PixelCharge> pixel_charges;
    Pulse total_pulse;
    for(auto& pixel_index_pulse : pixel_pulse_map) {
        auto index = pixel_index_pulse.first;
        auto pulse = pixel_index_pulse.second;

        // Sum all pulses for informational output:
        total_pulse += pulse;

        // Fill pixel charge histogram
        if(output_plots_) {
            h_induced_pixel_charge_->Fill(pulse.getCharge() / 1e3);
        }

        // Fill a graphs with the individual pixel pulses:
        if(output_pulsegraphs_) {
            auto step = pulse.getBinning();
            auto pulse_vec = pulse.getPulse();
            LOG(TRACE) << "Preparing pulse for pixel " << index << ", " << pulse_vec.size() << " bins of "
                       << Units::display(step, {"ps", "ns"})
                       << ", total charge: " << Units::display(pixel_index_pulse.second.getCharge(), "e");

            // Generate x-axis:
            std::vector<double> time(pulse_vec.size());
            // clang-format off
            std::generate(time.begin(), time.end(), [n = 0.0, step]() mutable {  auto now = n; n += step; return now; });
            // clang-format on

            std::string name = "pulse_ev" + std::to_string(event->number) + "_px" + std::to_string(index.x()) + "-" +
                               std::to_string(index.y());
            auto pulse_graph = new TGraph(static_cast<int>(pulse_vec.size()), &time[0], &pulse_vec[0]);
            pulse_graph->GetXaxis()->SetTitle("t [ns]");
            pulse_graph->GetYaxis()->SetTitle("Q_{ind} [e]");
            pulse_graph->SetTitle(("Induced charge in pixel (" + std::to_string(index.x()) + "," +
                                   std::to_string(index.y()) +
                                   "), Q_{tot} = " + std::to_string(pixel_index_pulse.second.getCharge()) + " e")
                                      .c_str());
            getROOTDirectory()->WriteTObject(pulse_graph, name.c_str());

            // Generate graphs of integrated charge over time:
            std::vector<double> charge_vec;
            double charge = 0;
            for(const auto& bin : pulse_vec) {
                charge += bin;
                charge_vec.push_back(charge);
            }

            name = "charge_ev" + std::to_string(event->number) + "_px" + std::to_string(index.x()) + "-" +
                   std::to_string(index.y());
            auto charge_graph = new TGraph(static_cast<int>(charge_vec.size()), &time[0], &charge_vec[0]);
            charge_graph->GetXaxis()->SetTitle("t [ns]");
            charge_graph->GetYaxis()->SetTitle("Q_{tot} [e]");
            charge_graph->SetTitle(("Accumulated induced charge in pixel (" + std::to_string(index.x()) + "," +
                                    std::to_string(index.y()) +
                                    "), Q_{tot} = " + std::to_string(pixel_index_pulse.second.getCharge()) + " e")
                                       .c_str());
            getROOTDirectory()->WriteTObject(charge_graph, name.c_str());
        }
        LOG(DEBUG) << "Charge on pixel " << index << " has " << pixel_charge_map[index].size() << " ancestors";

        // Store the pulse:
        pixel_charges.emplace_back(detector_->getPixel(index), std::move(pulse), pixel_charge_map[index]);
    }

    // Create a new message with pixel pulses and dispatch:
    auto pixel_charge_message = std::make_shared<PixelChargeMessage>(std::move(pixel_charges), detector_);
    event->dispatchMessage(pixel_charge_message);

    // Fill pixel charge histogram
    if(output_plots_) {
        h_total_induced_charge_->Fill(total_pulse.getCharge() / 1e3);
    }

    LOG(INFO) << "Total charge induced on all pixels: " << Units::display(total_pulse.getCharge(), "e");
}

void PulseTransferModule::finalize() {

    if(output_plots_) {
        // Write histograms
        LOG(TRACE) << "Writing output plots to file";
        h_induced_pixel_charge_->Write();
        h_total_induced_charge_->Write();
    }
}
