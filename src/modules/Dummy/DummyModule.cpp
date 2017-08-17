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

DummyModule::DummyModule(Configuration config, Messenger* messenger, GeometryManager* geoManager)
    : Module(config), geometryManager_(geoManager), config_(std::move(config)), messenger_(messenger) {

    // ... Implement ... (Typically bounds the required messages and optionally sets configuration defaults)
    LOG(TRACE) << "Initializing module " << getUniqueName();

    // Input required by this module
    messenger_->bindMulti(this, &DummyModule::messages_, MsgFlags::REQUIRED);
}

void DummyModule::init() {
    // Loop over detectors and do something
    std::vector<std::shared_ptr<Detector>> detectors = geometryManager_->getDetectors();
    unsigned long nDetectors = detectors.size();
    for(unsigned int det = 0; det < nDetectors; det++) {
        // Get the detector name
        std::string detectorName = detectors[det]->getName();
        LOG(DEBUG) << "Detector with name " << detectorName << std::endl;
    }
}

void DummyModule::run(unsigned int) {
    // ... Implement ... (Typically uses the configuration to execute function and outputs an message)
    LOG(TRACE) << "Running module " << getUniqueName();

    // Loop through all receieved messages
    unsigned int nMessages = 0;
    for(unsigned int messageNumber = 0; messageNumber < nMessages; messageNumber++) {

        // Get the message
        std::shared_ptr<PixelHitMessage> message = messages_[messageNumber];
        std::string detectorName = message->getDetector()->getName();
        LOG(DEBUG) << "Picked up " << message->getData().size() << " messages from detector " << detectorName << std::endl;
    }
}
