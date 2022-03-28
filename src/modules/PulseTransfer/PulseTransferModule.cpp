/**
 * @file
 * @brief Implementation of pulse transfer module
 * @copyright Copyright (c) 2019-2020 CERN and the Allpix Squared authors.
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
    : Module(config, detector), messenger_(messenger), detector_(detector) {

    // Set default value for config variables
    config_.setDefault("max_depth_distance", Units::get(5.0, "um"));
    config_.setDefault("collect_from_implant", false);

    config_.setDefault<double>("timestep", Units::get(0.01, "ns"));
    config_.setDefault<bool>("output_pulsegraphs", false);
    config_.setDefault<bool>("output_plots", config_.get<bool>("output_pulsegraphs"));
    config_.setDefault<int>("output_plots_scale", Units::get(30, "ke"));
    config_.setDefault<int>("output_plots_bins", 100);

    output_plots_ = config_.get<bool>("output_plots");
    output_pulsegraphs_ = config_.get<bool>("output_pulsegraphs");
    timestep_ = config_.get<double>("timestep");
    max_depth_distance_ = config_.get<double>("max_depth_distance");
    collect_from_implant_ = config_.get<bool>("collect_from_implant");

    // Enable multithreading of this module if multithreading is enabled and no per-event output plots are requested:
    // FIXME: Review if this is really the case or we can still use multithreading
    if(!output_pulsegraphs_) {
        allow_multithreading();
    } else {
        LOG(WARNING) << "Per-event pulse graphs requested, disabling parallel event processing";
    }

    messenger_->bindSingle<PropagatedChargeMessage>(this, MsgFlags::REQUIRED);
}

void PulseTransferModule::initialize() {

    if(output_plots_) {
        LOG(TRACE) << "Creating output plots";

        // Plot axis are in kilo electrons - convert from framework units!
        int maximum = static_cast<int>(Units::convert(config_.get<int>("output_plots_scale"), "ke"));
        auto nbins = config_.get<int>("output_plots_bins");

        // Create histograms if needed
        h_total_induced_charge_ = CreateHistogram<TH1D>(
            "inducedcharge", "total induced charge;induced charge [ke];events", nbins, -maximum, maximum);
        h_induced_pixel_charge_ = CreateHistogram<TH1D>(
            "pixelcharge", "induced charge per pixel;induced pixel charge [ke];pixels", nbins, -maximum, maximum);
        h_induced_pulses_ = CreateHistogram<TH2D>("pulses_induced",
                                                  "Induced charge per pixel;t [ns];Q_{ind} [e]",
                                                  nbins,
                                                  0,
                                                  10.,
                                                  nbins,
                                                  0,
                                                  config_.get<int>("output_plots_scale") / 0.5e3);
        h_integrated_pulses_ = CreateHistogram<TH2D>("pulses_integrated",
                                                     "Accumulated induced charge per pixel;t [ns];Q_{ind} [e]",
                                                     nbins,
                                                     0,
                                                     10.,
                                                     nbins,
                                                     0,
                                                     config_.get<int>("output_plots_scale"));
        p_induced_pulses_ = CreateHistogram<TProfile>(
            "pulses_induced_profile", "Induced charge per pixel;t [ns];Q_{ind} [e]", nbins, 0, 10.);
        p_integrated_pulses_ = CreateHistogram<TProfile>(
            "pulses_integrated_profile", "Accumulated induced charge per pixel;t [ns];Q_{ind} [e]", nbins, 0, 10.);
    }
}

void PulseTransferModule::run(Event* event) {
    auto propagated_message = messenger_->fetchMessage<PropagatedChargeMessage>(this, event);

    // Create map for all pixels: pulse and propagated charges
    std::map<Pixel::Index, Pulse> pixel_pulse_map;
    std::map<Pixel::Index, std::vector<const PropagatedCharge*>> pixel_charge_map;

    LOG(DEBUG) << "Received " << propagated_message->getData().size() << " propagated charge objects.";
    for(const auto& propagated_charge : propagated_message->getData()) {
        auto pulses = propagated_charge.getPulses();

        if(pulses.empty()) {
            LOG(TRACE) << "No pulse information available - producing pseudo-pulse from arrival time of charge carriers.";

            auto model = detector_->getModel();
            auto position = propagated_charge.getLocalPosition();

            // Ignore if outside depth range of implant
            if(std::fabs(position.z() - (model->getSensorCenter().z() + model->getSensorSize().z() / 2.0)) >
               max_depth_distance_) {
                LOG(TRACE) << "Skipping set of " << propagated_charge.getCharge() << " propagated charges at "
                           << Units::display(propagated_charge.getLocalPosition(), {"mm", "um"})
                           << " because their local position is not in implant range";
                continue;
            }

            // Find the nearest pixel
            auto xpixel = static_cast<int>(std::round(position.x() / model->getPixelSize().x()));
            auto ypixel = static_cast<int>(std::round(position.y() / model->getPixelSize().y()));

            // Ignore if out of pixel grid
            if(!detector_->isWithinPixelGrid(xpixel, ypixel)) {
                LOG(TRACE) << "Skipping set of " << propagated_charge.getCharge() << " propagated charges at "
                           << Units::display(propagated_charge.getLocalPosition(), {"mm", "um"})
                           << " because their nearest pixel (" << xpixel << "," << ypixel << ") is outside the grid";
                continue;
            }

            // Ignore if outside the implant region:
            if(collect_from_implant_) {
                if(detector_->getElectricFieldType() == FieldType::LINEAR) {
                    throw ModuleError(
                        "Charge collection from implant region should not be used with linear electric fields.");
                }

                if(!detector_->isWithinImplant(position)) {
                    LOG(TRACE) << "Skipping set of " << propagated_charge.getCharge() << " propagated charges at "
                               << Units::display(propagated_charge.getLocalPosition(), {"mm", "um"})
                               << " because it is outside the pixel implant.";
                    continue;
                }
            }

            Pixel::Index pixel_index(static_cast<unsigned int>(xpixel), static_cast<unsigned int>(ypixel));

            // Generate pseudo-pulse:
            Pulse pulse(timestep_);
            try {
                pulse.addCharge(static_cast<double>(propagated_charge.getSign() * propagated_charge.getCharge()),
                                propagated_charge.getLocalTime());
            } catch(const PulseBadAllocException& e) {
                LOG(ERROR) << e.what() << std::endl
                           << "Ignoring pulse contribution at time "
                           << Units::display(propagated_charge.getLocalTime(), {"ms", "us", "ns"});
            }
            pixel_pulse_map[pixel_index] += pulse;

            auto px = pixel_charge_map[pixel_index];
            // For each pulse, store the corresponding propagated charges to preserve history:
            if(std::find(px.begin(), px.end(), &propagated_charge) == px.end()) {
                pixel_charge_map[pixel_index].emplace_back(&propagated_charge);
            }
        } else {
            LOG(TRACE) << "Found pulse information";
            LOG_ONCE(INFO) << "Pulses available - settings \"timestep\", \"max_depth_distance\" and "
                              "\"collect_from_implant\" have no effect";

            for(auto& [pixel_index, pulse] : pulses) {
                // Accumulate all pulses from input message data:
                pixel_pulse_map[pixel_index] += pulse;

                auto px = pixel_charge_map[pixel_index];
                // For each pulse, store the corresponding propagated charges to preserve history:
                if(std::find(px.begin(), px.end(), &propagated_charge) == px.end()) {
                    pixel_charge_map[pixel_index].emplace_back(&propagated_charge);
                }
            }
        }
    }

    // Create vector of pixel pulses to return for this detector
    std::vector<PixelCharge> pixel_charges;
    Pulse total_pulse;
    for(auto& [index, pulse] : pixel_pulse_map) {
        // Sum all pulses for informational output:
        total_pulse += pulse;

        // Fill pixel charge histogram
        if(output_plots_) {
            h_induced_pixel_charge_->Fill(pulse.getCharge() / 1e3);

            auto step = pulse.getBinning();
            auto pulse_vec = pulse.getPulse();
            double charge = 0;

            for(auto bin = pulse_vec.begin(); bin != pulse_vec.end(); ++bin) {
                auto time = step * static_cast<double>(std::distance(pulse_vec.begin(), bin));
                h_induced_pulses_->Fill(time, *bin);
                p_induced_pulses_->Fill(time, *bin);

                charge += *bin;
                h_integrated_pulses_->Fill(time, charge);
                p_integrated_pulses_->Fill(time, charge);
            }
        }

        // Fill a graphs with the individual pixel pulses:
        if(output_pulsegraphs_) {
            create_pulsegraphs(event->number, index, pulse);
        }
        LOG(DEBUG) << "Charge on pixel " << index << " has " << pixel_charge_map[index].size() << " ancestors";

        // Store the pulse:
        pixel_charges.emplace_back(detector_->getPixel(index), std::move(pulse), pixel_charge_map[index]);
    }

    if(output_pulsegraphs_) {
        std::string name = "chargemap_ev" + std::to_string(event->number);
        auto size = detector_->getModel()->getNPixels();
        std::string title =
            "Map of accumulated induced charge in event " + std::to_string(event->number) + ";x (pixels);y (pixels);charge";
        auto* charge_map = new TH2D(name.c_str(),
                                    title.c_str(),
                                    static_cast<int>(size.x()),
                                    -0.5,
                                    static_cast<int>(size.x()) - 0.5,
                                    static_cast<int>(size.y()),
                                    -0.5,
                                    static_cast<int>(size.y()) - 0.5);

        for(auto& [index, pulse] : pixel_pulse_map) {
            charge_map->Fill(index.x(), index.y(), pulse.getCharge());
        }
        getROOTDirectory()->WriteTObject(charge_map, name.c_str());
    }

    // Create a new message with pixel pulses and dispatch:
    auto pixel_charge_message = std::make_shared<PixelChargeMessage>(std::move(pixel_charges), detector_);
    messenger_->dispatchMessage(this, pixel_charge_message, event);

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
        h_induced_pulses_->Write();
        h_integrated_pulses_->Write();
        p_induced_pulses_->Write();
        p_integrated_pulses_->Write();
    }
}

void PulseTransferModule::create_pulsegraphs(uint64_t event_num, const Pixel::Index& index, const Pulse& pulse) const {
    auto step = pulse.getBinning();
    auto pulse_vec = pulse.getPulse();
    LOG(TRACE) << "Preparing pulse for pixel " << index << ", " << pulse_vec.size() << " bins of "
               << Units::display(step, {"ps", "ns"}) << ", total charge: " << Units::display(pulse.getCharge(), "e");

    // Generate x-axis:
    std::vector<double> time(pulse_vec.size());
    // clang-format off
    std::generate(time.begin(), time.end(), [n = 0.0, step]() mutable { auto now = n; n += step; return now; });
    // clang-format on

    std::string name =
        "pulse_ev" + std::to_string(event_num) + "_px" + std::to_string(index.x()) + "-" + std::to_string(index.y());
    auto* pulse_graph = new TGraph(static_cast<int>(pulse_vec.size()), &time[0], &pulse_vec[0]);
    pulse_graph->GetXaxis()->SetTitle("t [ns]");
    pulse_graph->GetYaxis()->SetTitle("Q_{ind} [e]");
    pulse_graph->SetTitle(("Induced charge per unit step time in pixel (" + std::to_string(index.x()) + "," +
                           std::to_string(index.y()) + "), Q_{tot} = " + Units::display(pulse.getCharge(), {"e", "ke"}) +
                           " (" + Units::display(pulse.getCharge(), "fC") + ")")
                              .c_str());
    getROOTDirectory()->WriteTObject(pulse_graph, name.c_str());

    std::vector<double> abs_vec = pulse_vec;
    std::for_each(abs_vec.begin(), abs_vec.end(), [](auto& bin) { bin = std::fabs(bin); });
    name = "pulse_abs_ev" + std::to_string(event_num) + "_px" + std::to_string(index.x()) + "-" + std::to_string(index.y());
    auto* pulse_abs_graph = new TGraph(static_cast<int>(abs_vec.size()), &time[0], &abs_vec[0]);
    pulse_abs_graph->GetXaxis()->SetTitle("t [ns]");
    pulse_abs_graph->GetYaxis()->SetTitle("|Q_{ind}| [e]");
    pulse_abs_graph->SetTitle(("Absolute induced charge per unit step time in pixel (" + std::to_string(index.x()) + "," +
                               std::to_string(index.y()) +
                               "), |Q_{tot}| = " + Units::display(pulse.getCharge(), {"e", "ke"}) + " (" +
                               Units::display(pulse.getCharge(), "fC") + ")")
                                  .c_str());
    getROOTDirectory()->WriteTObject(pulse_abs_graph, name.c_str());

    std::vector<double> current_vec = pulse_vec;
    // Convert charge bins to current in uA
    std::for_each(current_vec.begin(), current_vec.end(), [step](auto& bin) {
        bin = static_cast<double>(Units::convert(bin, "fC") / Units::convert(step, "ns"));
    });

    // Generate graphs of induced current over time:
    name = "current_ev" + std::to_string(event_num) + "_px" + std::to_string(index.x()) + "-" + std::to_string(index.y());
    auto* current_graph = new TGraph(static_cast<int>(current_vec.size()), &time[0], &current_vec[0]);
    current_graph->GetXaxis()->SetTitle("t [ns]");
    current_graph->GetYaxis()->SetTitle("I_{ind} [uA]");
    current_graph->SetTitle(("Induced current in pixel (" + std::to_string(index.x()) + "," + std::to_string(index.y()) +
                             "), Q_{tot} = " + Units::display(pulse.getCharge(), {"e", "ke"}) + " (" +
                             Units::display(pulse.getCharge(), "fC") + ")")
                                .c_str());
    getROOTDirectory()->WriteTObject(current_graph, name.c_str());

    // Generate graphs of integrated charge over time:
    std::vector<double> charge_vec;
    double charge = 0;
    for(const auto& bin : pulse_vec) {
        charge += bin;
        charge_vec.push_back(charge);
    }

    name = "charge_ev" + std::to_string(event_num) + "_px" + std::to_string(index.x()) + "-" + std::to_string(index.y());
    auto* charge_graph = new TGraph(static_cast<int>(charge_vec.size()), &time[0], &charge_vec[0]);
    charge_graph->GetXaxis()->SetTitle("t [ns]");
    charge_graph->GetYaxis()->SetTitle("Q_{tot} [e]");
    charge_graph->SetTitle(("Accumulated induced charge in pixel (" + std::to_string(index.x()) + "," +
                            std::to_string(index.y()) + "), Q_{tot} = " + Units::display(pulse.getCharge(), {"e", "ke"}) +
                            " (" + Units::display(pulse.getCharge(), "fC") + ")")
                               .c_str());
    getROOTDirectory()->WriteTObject(charge_graph, name.c_str());
}
