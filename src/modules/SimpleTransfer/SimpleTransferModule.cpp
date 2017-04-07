#include "SimpleTransferModule.hpp"

#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

#include "core/config/InvalidValueError.hpp"
#include "core/geometry/PixelDetectorModel.hpp"
#include "core/utils/log.h"
#include "core/utils/unit.h"

// use the allpix namespace within this file
using namespace allpix;

// set the name of the module
const std::string SimpleTransferModule::name = "SimpleTransfer";

// constructor to load the module
SimpleTransferModule::SimpleTransferModule(Configuration config, Messenger* messenger, std::shared_ptr<Detector> detector)
    : Module(detector), config_(std::move(config)), detector_(std::move(detector)), propagated_message_() {
    // fetch propagated deposits for single detector
    messenger->bindSingle(this, &SimpleTransferModule::propagated_message_);
}

// run method that does the main computations for the module
void SimpleTransferModule::run() {
    // skip if this detector did not get any deposits
    if(propagated_message_ == nullptr) {
        return;
    }

    // get detector model
    auto model = std::dynamic_pointer_cast<PixelDetectorModel>(detector_->getModel());
    if(model == nullptr) {
        // FIXME: exception can be more appropriate here
        LOG(CRITICAL) << "Detector " << detector_->getName()
                      << " is not a PixelDetectorModel: ignored as other types are currently unsupported!";
        return;
    }

    // propagate all deposits
    for(auto& deposit : propagated_message_->getData()) {

        auto position = deposit.getPosition();
        int xpixel = static_cast<int>(std::round(position.x() / model->getPixelSizeX()));
        int ypixel = static_cast<int>(std::round(position.y() / model->getPixelSizeY()));
        // NOTE: z position is ignored here
        LOG(DEBUG) << "set of " << deposit.getCharge() << " propagated charges at " << deposit.getPosition() << std::endl
                   << "location is on pixel (" << xpixel << "," << ypixel << ")";
    }
}
