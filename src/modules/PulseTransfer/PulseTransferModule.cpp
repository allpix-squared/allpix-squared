/**
 * @file
 * @brief Implementation of PulseTransfer module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "PulseTransferModule.hpp"
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
    : Module(config, detector), detector_(detector), messenger_(messenger) {

    config_.setDefault<bool>("output_pulsegraphs", false);
    config_.setDefault<bool>("output_plots", config_.get<bool>("output_pulsegraphs"));

    output_plots_ = config_.get<bool>("output_plots");
    output_pulsegraphs_ = config_.get<bool>("output_pulsegraphs");

    messenger_->bindSingle(this, &PulseTransferModule::message_, MsgFlags::REQUIRED);
}

void PulseTransferModule::run(unsigned int event_num) {

    // Create map for all pixels: pulse and propagated charges
    std::map<Pixel::Index, Pulse> pixel_pulse_map;
    std::map<Pixel::Index, std::vector<const PropagatedCharge*>> pixel_charge_map;

    LOG(DEBUG) << "Received " << message_->getData().size() << " propagated charge objects.";
    for(const auto& propagated_charge : message_->getData()) {
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

        // Fill a graphs with the individual pixel pulses:
        if(output_pulsegraphs_) {
            auto step = pulse.getBinning();
            auto pulse_vec = pulse.getPulse();

            std::string name =
                "pulse_px" + std::to_string(index.x()) + "-" + std::to_string(index.y()) + "_" + std::to_string(event_num);

            // Generate x-axis:
            std::vector<double> time(pulse_vec.size());
            // clang-format off
            std::generate(time.begin(), time.end(), [n = 0.0, step]() mutable { return n += step; });
            // clang-format on

            auto pulse_graph = new TGraph(static_cast<int>(pulse_vec.size()), &time[0], &pulse_vec[0]);
            pulse_graph->GetXaxis()->SetTitle("t [ns]");
            pulse_graph->GetYaxis()->SetTitle("Q_{ind} [e]");
            pulse_graph->SetTitle(("Induced charge in pixel (" + std::to_string(index.x()) + "," +
                                   std::to_string(index.y()) +
                                   "), Q_{tot} = " + std::to_string(pixel_index_pulse.second.getCharge()) + " e")
                                      .c_str());
            pulse_graph->Write(name.c_str());
        }
        LOG(DEBUG) << "Charge on pixel " << index << " has " << pixel_charge_map[index].size() << " ancestors";

        // Store the pulse:
        pixel_charges.emplace_back(detector_->getPixel(index), std::move(pulse), pixel_charge_map[index]);
    }

    // Create a new message with pixel pulses and dispatch:
    auto pixel_charge_message = std::make_shared<PixelChargeMessage>(std::move(pixel_charges), detector_);
    messenger_->dispatchMessage(this, pixel_charge_message);

    LOG(INFO) << "Total charge induced on all pixels: " << Units::display(total_pulse.getCharge(), "e");
}
