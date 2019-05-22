/**
 * @file
 * @brief Implementation of [Dummy] module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "DummyModule.hpp"

#include <string>
#include <utility>

#include "core/utils/log.h"

using namespace allpix;

DummyModule::DummyModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager)
    : Module(config), geo_manager_(geo_manager) {

    // ... Implement ... (Typically bounds the required messages and optionally sets configuration defaults)
    // Input required by this module
    messenger->bindMulti<DummyModule, PixelHitMessage>(this);
}

void DummyModule::init(std::mt19937_64&) {
    // Loop over detectors and do something
    std::vector<std::shared_ptr<Detector>> detectors = geo_manager_->getDetectors();
    for(auto& detector : detectors) {
        // Get the detector name
        std::string detectorName = detector->getName();
        LOG(DEBUG) << "Detector with name " << detectorName;
    }
}

void DummyModule::run(Event* event) const {
    auto messages = event->fetchMultiMessage<PixelHitMessage>();
    // ... Implement ... (Typically uses the configuration to execute function and outputs an message)
    // Loop through all receieved messages and print some information
    for(auto& message : messages) {
        std::string detectorName = message->getDetector()->getName();
        LOG(DEBUG) << "Picked up " << message->getData().size() << " objects from detector " << detectorName;
    }
}

// check if the required messages are present in the event
bool DummyModule::isSatisfied(Event* event) const {
    try {
        auto messages = event->fetchMultiMessage<PixelHitMessage>();
    } catch (std::out_of_range&) {
        return false;
    }

    return true;
}
