/**
 * @file
 * @brief Implementation of simple charge transfer module
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "SimpleTransferModule.hpp"

#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

#include "core/config/exceptions.h"
#include "core/utils/log.h"
#include "core/utils/unit.h"
#include "tools/ROOT.h"

#include "objects/PixelCharge.hpp"

using namespace allpix;

SimpleTransferModule::SimpleTransferModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector)
    : Module(config, detector), messenger_(messenger), detector_(std::move(detector)) {
    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();

    // Set default value for the maximum depth distance to transfer
    config_.setDefault("max_depth_distance", Units::get(5.0, "um"));

    // By default, collect from the full sensor surface, not the implant region
    config_.setDefault("collect_from_implant", false);

    // Plotting parameters
    config_.setDefault<bool>("output_plots", false);
    config_.setDefault<double>("output_plots_step", Units::get(0.1, "ns"));
    config_.setDefault<double>("output_plots_range", Units::get(100, "ns"));

    // Save detector model
    model_ = detector_->getModel();

    // Cache config parameters:
    max_depth_distance_ = config_.get<double>("max_depth_distance");
    collect_from_implant_ = config_.get<bool>("collect_from_implant");

    // Cache flag for output plots:
    output_plots_ = config_.get<bool>("output_plots");

    // Require propagated deposits for single detector
    messenger_->bindSingle<PropagatedChargeMessage>(this, MsgFlags::REQUIRED);
}

void SimpleTransferModule::initialize() {

    auto model = detector_->getModel();
    if(collect_from_implant_) {
        if(model->getImplants().empty()) {
            throw InvalidValueError(config_,
                                    "collect_from_implant",
                                    "Detector model does not have implants defined, but collection requested from implants");
        }
        if(detector_->getElectricFieldType() == FieldType::LINEAR) {
            throw ModuleError("Charge collection from implant region should not be used with linear electric fields.");
        }
        LOG(INFO) << "Collecting charges from implants";
    } else if(!model->getImplants().empty()) {
        LOG(WARNING) << "Detector " << detector_->getName() << " of type " << model->getType()
                     << " has implants defined but collecting charge carriers from full sensor surface";
    }

    if(output_plots_) {
        auto time_bins =
            static_cast<int>(config_.get<double>("output_plots_range") / config_.get<double>("output_plots_step"));
        drift_time_histo = CreateHistogram<TH1D>("drift_time_histo",
                                                 "Charge carrier arrival time;t[ns];charge carriers",
                                                 time_bins,
                                                 0.,
                                                 config_.get<double>("output_plots_range"));
    }
}

void SimpleTransferModule::run(Event* event) {
    auto propagated_message = messenger_->fetchMessage<PropagatedChargeMessage>(this, event);

    // Find corresponding pixels for all propagated charges
    LOG(TRACE) << "Transferring charges to pixels";
    unsigned int transferred_charges_count = 0;
    std::map<Pixel::Index, std::vector<const PropagatedCharge*>> pixel_map;
    for(const auto& propagated_charge : propagated_message->getData()) {
        auto position = propagated_charge.getLocalPosition();

        if(collect_from_implant_) {
            // Ignore if outside the implant region:
            auto implant = model_->isWithinImplant(position);
            if(!implant.has_value()) {
                LOG(TRACE) << "Skipping set of " << propagated_charge.getCharge() << " propagated charges at "
                           << Units::display(propagated_charge.getLocalPosition(), {"mm", "um"})
                           << " because their local position is outside the pixel implant";
                continue;
            }
            if(implant->getType() != DetectorModel::Implant::Type::FRONTSIDE) {
                LOG(TRACE) << "Skipping set of " << propagated_charge.getCharge() << " propagated charges at "
                           << Units::display(propagated_charge.getLocalPosition(), {"mm", "um"})
                           << " because the pixel implant is located at " << allpix::to_string(implant->getType());
                continue;
            }
        } else if(std::fabs(position.z() - (model_->getSensorCenter().z() + model_->getSensorSize().z() / 2.0)) >
                  max_depth_distance_) {
            // Ignore if not close to the sensor surface:
            LOG(TRACE) << "Skipping set of " << propagated_charge.getCharge() << " propagated charges at "
                       << Units::display(propagated_charge.getLocalPosition(), {"mm", "um"})
                       << " because their local position is not near sensor surface";
            continue;
        }

        // Find the nearest pixel
        auto [xpixel, ypixel] = model_->getPixelIndex(position);

        // Ignore if out of pixel grid
        if(!model_->isWithinMatrix(xpixel, ypixel)) {
            LOG(TRACE) << "Skipping set of " << propagated_charge.getCharge() << " propagated charges at "
                       << Units::display(propagated_charge.getLocalPosition(), {"mm", "um"})
                       << " because their nearest pixel (" << xpixel << "," << ypixel << ") is outside the grid";
            continue;
        }

        Pixel::Index pixel_index(xpixel, ypixel);

        // Update statistics
        transferred_charges_count += propagated_charge.getCharge();

        if(output_plots_) {
            drift_time_histo->Fill(propagated_charge.getGlobalTime(), propagated_charge.getCharge());
        }

        LOG(TRACE) << "Set of " << propagated_charge.getCharge() << " propagated charges at "
                   << Units::display(propagated_charge.getLocalPosition(), {"mm", "um"}) << " brought to pixel "
                   << pixel_index;

        // Add the pixel the list of hit pixels
        pixel_map[pixel_index].emplace_back(&propagated_charge);
    }

    // Create pixel charges
    LOG(TRACE) << "Combining charges at same pixel";
    std::vector<PixelCharge> pixel_charges;
    for(auto& pixel_index_charge : pixel_map) {
        long charge = 0;
        for(auto& propagated_charge : pixel_index_charge.second) {
            charge += propagated_charge->getSign() * propagated_charge->getCharge();
        }

        // Get pixel object from detector
        auto pixel = detector_->getPixel(pixel_index_charge.first.x(), pixel_index_charge.first.y());

        pixel_charges.emplace_back(pixel, charge, pixel_index_charge.second);
        LOG(DEBUG) << "Set of " << charge << " charges combined at " << pixel.getIndex();
    }

    // Writing summary and update statistics
    LOG(INFO) << "Transferred " << transferred_charges_count << " charges to " << pixel_map.size() << " pixels";
    total_transferred_charges_ += transferred_charges_count;

    // Dispatch message of pixel charges
    auto pixel_message = std::make_shared<PixelChargeMessage>(pixel_charges, detector_);
    messenger_->dispatchMessage(this, pixel_message, event);
}

void SimpleTransferModule::finalize() {
    // Print statistics
    LOG(INFO) << "Transferred total of " << total_transferred_charges_ << " charges";

    if(output_plots_) {
        drift_time_histo->Write();
    }
}
