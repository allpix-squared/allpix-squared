/**
 * @file
 * @brief Implementation of Dummy module
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "DummyModule.hpp"

#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/messenger/delegates.h"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"
#include "core/utils/log.h"
#include "objects/PixelHit.hpp"

using namespace allpix;

DummyModule::DummyModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager)
    : Module(config), geo_manager_(geo_manager), messenger_(messenger) {

    // Allow multithreading of the simulation. Only enabled if this module is thread-safe. See manual for more details.
    // allow_multithreading();

    // Set a default for a configuration parameter, this will be used if no user configuration is provided:
    config_.setDefault<int>("setting", 13);

    // Parsing of the parameter "setting" into a member variable for later use:
    setting_ = config_.get<int>("setting");

    // Messages: register this module with the central messenger to request a certaintype of input messages:
    messenger_->bindMulti<PixelHitMessage>(this, MsgFlags::REQUIRED);
}

void DummyModule::initialize() {

    // Loop over detectors and perform some initialization or similar:
    for(auto& detector : geo_manager_->getDetectors()) {
        // In this simple case we just print the name of this detector:
        LOG(DEBUG) << "Detector with name " << detector->getName();
    }
}

void DummyModule::run(Event* event) {

    // Messages: Fetch the (previously registered) messages for this event from the messenger:
    auto messages = messenger_->fetchMultiMessage<PixelHitMessage>(this, event);

    // Messages: Loop through all received messages
    for(auto& message : messages) {
        // Print the name of the detector for which this particular message has been dispatched:
        LOG(DEBUG) << "Picked up " << message->getData().size() << " objects from detector "
                   << message->getDetector()->getName();
    }
}

void DummyModule::finalize() {
    // Possibly perform finalization of the module - if not, this method does not need to be implemented and can be removed!
    LOG(INFO) << "Successfully finalized!";
}
