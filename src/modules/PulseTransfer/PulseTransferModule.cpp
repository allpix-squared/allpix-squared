/**
 * @file
 * @brief Implementation of PulseTransfer module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "PulseTransferModule.hpp"
#include "objects/PixelCharge.hpp"

#include <string>
#include <utility>

#include <TAxis.h>
#include <TGraph.h>

#include "core/utils/log.h"

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

    // Create map for all pixels
    std::map<Pixel::Index, Pulse> pixel_map;

    Pulse total_pulse;

    for(const auto& prop : message_->getData()) {
        auto pulses = prop.getPulses();
        pixel_map =
            std::accumulate(pulses.begin(), pulses.end(), pixel_map, [](std::map<Pixel::Index, Pulse>& m, const auto& p) {
                return (m[p.first] += p.second, m);
            });
    }

    // Create vector of pixel pulses to return for this detector
    std::vector<PixelCharge> pixel_charges;
    for(auto& pixel_index_pulse : pixel_map) {
        total_pulse += pixel_index_pulse.second;

        // Fill a graphs with the individual pixel pulses:
        if(output_pulsegraphs_) {
            auto index = pixel_index_pulse.first;
            auto pulse = pixel_index_pulse.second.getPulse();
            auto step = pixel_index_pulse.second.getBinning();

            std::string name =
                "pulse_px" + std::to_string(index.x()) + "-" + std::to_string(index.y()) + "_" + std::to_string(event_num);

            // Generate x-axis:
            std::vector<double> time(pulse.size());
            std::generate(time.begin(), time.end(), [ n = 0.0, step ]() mutable { return n += step; });

            auto pulse_graph = new TGraph(static_cast<int>(pulse.size()), &time[0], &pulse[0]);
            pulse_graph->GetXaxis()->SetTitle("t [ns]");
            pulse_graph->GetYaxis()->SetTitle("Q_{ind} [e]");
            pulse_graph->SetTitle(("Induced charge in pixel (" + std::to_string(index.x()) + "," +
                                   std::to_string(index.y()) +
                                   "), Q_{tot} = " + std::to_string(pixel_index_pulse.second.getCharge()) + " e")
                                      .c_str());
            pulse_graph->Write(name.c_str());
        }

        // Store the pulse:
        pixel_charges.emplace_back(detector_->getPixel(pixel_index_pulse.first), std::move(pixel_index_pulse.second));
    }

    // Create a new message with pixel pulses
    auto pixel_charge_message = std::make_shared<PixelChargeMessage>(std::move(pixel_charges), detector_);

    // Dispatch the message with pixel charges
    messenger_->dispatchMessage(this, pixel_charge_message);

    LOG(INFO) << "Total charge induced on all pixels: " << Units::display(total_pulse.getCharge(), "e");
}
