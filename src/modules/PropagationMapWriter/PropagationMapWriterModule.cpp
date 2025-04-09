/**
 * @file
 * @brief Implementation of PropagationMapWriter module
 *
 * @copyright Copyright (c) 2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "PropagationMapWriterModule.hpp"

#include <string>
#include <utility>

#include "core/utils/log.h"

using namespace allpix;

PropagationMapWriterModule::PropagationMapWriterModule(Configuration& config,
                                                       Messenger* messenger,
                                                       std::shared_ptr<Detector> detector)
    : Module(config, detector), detector_(std::move(detector)), messenger_(messenger) {

    // Allow multithreading of the simulation. Only enabled if this module is thread-safe. See manual for more details.
    // allow_multithreading();

    model_ = detector_->getModel();

    // Messages: register this module with the central messenger to request a certaintype of input messages:
    messenger_->bindSingle<PixelChargeMessage>(this, MsgFlags::REQUIRED);
}

void PropagationMapWriterModule::initialize() {

    // In this simple case we just print the name of this detector:
    LOG(DEBUG) << "Detector with name " << detector_->getName();
}

void PropagationMapWriterModule::run(Event* event) {

    // Messages: Fetch the (previously registered) messages for this event from the messenger:
    auto message = messenger_->fetchMessage<PixelChargeMessage>(this, event);

    // Loop over all pixel charges
    for(const auto& pixel_charge : message->getData()) {
        // Obtain deposited charges:
        for(const auto& propagated_charge : pixel_charge.getPropagatedCharges()) {
            const auto& deposit = propagated_charge->getDepositedCharge();

            // Fetch initial position and pixel index:
            const auto position = deposit->getLocalPosition();
            const auto [xpixel, ypixel] = model_->getPixelIndex(position);

            const auto final = pixel_charge.getIndex();

            LOG(TRACE) << "Deposited charge at " << deposit->getLocalPosition() << " from pixel " << xpixel << "," << ypixel
                       << " ended on pixel " << final << ", relative: " << (final.x() - xpixel) << ","
                       << (final.y() - ypixel);
        }
    }
}

void PropagationMapWriterModule::finalize() {
    // Possibly perform finalization of the module - if not, this method does not need to be implemented and can be removed!
    LOG(INFO) << "Successfully finalized!";
}
