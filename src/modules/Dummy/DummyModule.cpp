/**
 * @file
 * @brief Implementation of [Dummy] module
 * @copyright MIT License
 */

#include "DummyModule.hpp"

#include <string>
#include <utility>

#include "core/utils/log.h"

using namespace allpix;

DummyModule::DummyModule(Configuration config, Messenger* messenger, GeometryManager* geo_manager)
    : Module(config), geo_manager_(geo_manager), config_(std::move(config)), messenger_(messenger) {

    // ... Implement ... (Typically bounds the required messages and optionally sets configuration defaults)
    LOG(TRACE) << "Initializing module " << getUniqueName();

    // Input required by this module
    messenger_->bindMulti(this, &DummyModule::messages_, MsgFlags::REQUIRED);
}

void DummyModule::init() {
    // Loop over detectors and do something
    std::vector<std::shared_ptr<Detector>> detectors = geo_manager_->getDetectors();
    for(auto& detector : detectors) {
        // Get the detector name
        std::string detectorName = detector->getName();
        LOG(DEBUG) << "Detector with name " << detectorName;
    }
}

void DummyModule::run(unsigned int) {
    // ... Implement ... (Typically uses the configuration to execute function and outputs an message)
    // Loop through all receieved messages and print some information
    for(auto& message : messages_) {
        std::string detectorName = message->getDetector()->getName();
        LOG(DEBUG) << "Picked up " << message->getData().size() << " objects from detector " << detectorName;
    }
}
