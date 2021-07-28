/**
 * @file
 * @brief Implementation of InducedTransfer module
 * @copyright Copyright (c) 2019-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "InducedTransferModule.hpp"

#include <string>
#include <utility>

#include "core/geometry/exceptions.h"
#include "core/module/Event.hpp"
#include "core/utils/log.h"
#include "objects/PixelCharge.hpp"

using namespace allpix;
using namespace ROOT::Math;

InducedTransferModule::InducedTransferModule(Configuration& config,
                                             Messenger* messenger,
                                             const std::shared_ptr<Detector>& detector)
    : Module(config, detector), messenger_(messenger), detector_(detector) {
    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();

    using XYVectorInt = DisplacementVector2D<Cartesian2D<int>>;

    // Save detector model
    model_ = detector_->getModel();

    // Set default value for config variables and store value
    config_.setDefault<XYVectorInt>("induction_matrix", XYVectorInt(3, 3));
    matrix_ = config_.get<XYVectorInt>("induction_matrix");

    // Require propagated deposits for single detector
    messenger_->bindSingle<PropagatedChargeMessage>(this, MsgFlags::REQUIRED);
}

void InducedTransferModule::initialize() {

    // This module requires a weighting potential - otherwise everything is lost...
    if(!detector_->hasWeightingPotential()) {
        throw ModuleError("This module requires a weighting potential.");
    }
}

void InducedTransferModule::run(Event* event) {
    auto propagated_message = messenger_->fetchMessage<PropagatedChargeMessage>(this, event);

    // Calculate induced charge by total motion of charge carriers
    LOG(TRACE) << "Calculating induced charge on pixels";
    bool found_electrons = false, found_holes = false;

    std::map<Pixel::Index, std::vector<std::pair<double, const PropagatedCharge*>>> pixel_map;
    for(const auto& propagated_charge : propagated_message->getData()) {

        // Make sure both electrons and holes are present in the input data
        if(propagated_charge.getType() == CarrierType::ELECTRON) {
            found_electrons = true;
        } else if(propagated_charge.getType() == CarrierType::HOLE) {
            found_holes = true;
        }

        const auto* deposited_charge = propagated_charge.getDepositedCharge();

        // Get start and end point by looking at deposited and propagated charge local positions
        auto position_end = propagated_charge.getLocalPosition();
        auto position_start = deposited_charge->getLocalPosition();

        // Find the nearest pixel
        Pixel::Index pixel;
        try {
            pixel = model_->findPixel(position_end);
        }
        // Ignore if out of pixel grid
        catch(PixelOutsideGridException& e) {
            LOG(TRACE) << "Skipping set of " << propagated_charge.getCharge() << " propagated charges at "
                       << Units::display(position_end, {"mm", "um"}) << " because their nearest pixel " << pixel
                       << " is outside the grid";
            continue;
        }

        auto xpixel = static_cast<int>(pixel.x());
        auto ypixel = static_cast<int>(pixel.y());
        LOG(TRACE) << "Calculating induced charge from carriers below pixel " << pixel << ", moved from "
                   << Units::display(position_start, {"um", "mm"}) << " to " << Units::display(position_end, {"um", "mm"})
                   << ", " << Units::display(propagated_charge.getGlobalTime() - deposited_charge->getGlobalTime(), "ns");

        // Loop over NxN pixels:
        for(int x = xpixel - matrix_.x() / 2; x <= xpixel + matrix_.x() / 2; x++) {
            for(int y = ypixel - matrix_.y() / 2; y <= ypixel + matrix_.y() / 2; y++) {
                // Ignore if out of pixel grid
                if(!detector_->getModel()->isWithinPixelGrid(x, y)) {
                    LOG(TRACE) << "Pixel (" << x << "," << y << ") skipped, outside the grid";
                    continue;
                }

                Pixel::Index pixel_index(static_cast<unsigned int>(x), static_cast<unsigned int>(y));
                auto ramo_end = detector_->getWeightingPotential(position_end, pixel_index);
                auto ramo_start = detector_->getWeightingPotential(position_start, pixel_index);

                // Induced charge on electrode is q_int = q * (phi(x1) - phi(x0))
                auto induced = static_cast<double>(propagated_charge.getSign() * propagated_charge.getCharge()) *
                               (ramo_end - ramo_start);
                LOG(TRACE) << "Pixel " << pixel_index << " dPhi = " << (ramo_end - ramo_start) << ", induced "
                           << propagated_charge.getType() << " q = " << Units::display(induced, "e");

                // Add the pixel the list of hit pixels
                pixel_map[pixel_index].emplace_back(induced, &propagated_charge);
            }
        }
    }

    // Send an error message if this even only contained one of the two carrier types
    if(!found_electrons || !found_holes) {
        LOG(ERROR) << "Did not find charge carriers of type \"" << (found_electrons ? "holes" : "electrons")
                   << "\" in this event." << std::endl
                   << "This will cause wrong calculation of induced charge";
    }

    // Create pixel charges
    LOG(TRACE) << "Combining charges at same pixel";
    std::vector<PixelCharge> pixel_charges;
    for(auto& pixel_index_charge : pixel_map) {
        double charge = 0;
        std::vector<const PropagatedCharge*> prop_charges;
        for(auto& prop_pair : pixel_index_charge.second) {
            charge += prop_pair.first;
            prop_charges.push_back(prop_pair.second);
        }

        // Get pixel object from detector
        auto pixel = detector_->getPixel(pixel_index_charge.first.x(), pixel_index_charge.first.y());

        pixel_charges.emplace_back(pixel, std::round(charge), prop_charges);
        LOG(DEBUG) << "Set of " << charge << " charges combined at " << pixel.getIndex();
    }

    // Dispatch message of pixel charges
    auto pixel_message = std::make_shared<PixelChargeMessage>(pixel_charges, detector_);
    messenger_->dispatchMessage(this, pixel_message, event);
}
