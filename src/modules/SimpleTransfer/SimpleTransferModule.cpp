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

#include "objects/PixelCharge.hpp"

using namespace allpix;

// set the name of the module
const std::string SimpleTransferModule::name = "SimpleTransfer";

// constructor to load the module
SimpleTransferModule::SimpleTransferModule(Configuration config, Messenger* messenger, std::shared_ptr<Detector> detector)
    : Module(detector), config_(std::move(config)), messenger_(messenger), detector_(std::move(detector)),
      propagated_message_() {
    // fetch propagated deposits for single detector
    messenger->bindSingle(this, &SimpleTransferModule::propagated_message_);
}

// compare two pixels for the pixel map
struct pixel_cmp {
    bool operator()(const PixelCharge::Pixel& p1, const PixelCharge::Pixel& p2) const {
        if(p1.x() == p2.x()) {
            return p1.y() < p2.y();
        }
        return p1.x() < p2.x();
    }
};

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
        LOG(ERROR) << "Detector " << detector_->getName()
                   << " is not a PixelDetectorModel: ignored as other types are currently unsupported!";
        return;
    }

    // find pixels for all propagated charges
    LOG(INFO) << "Transferring charges to pixels";
    std::map<PixelCharge::Pixel, std::vector<PropagatedCharge>, pixel_cmp> pixel_map;
    for(auto& propagated_charge : propagated_message_->getData()) {
        auto position = propagated_charge.getPosition();
        // find the nearest pixel
        // NOTE: z position is ignored here
        int xpixel = static_cast<int>(std::round(position.x() / model->getPixelSizeX()));
        int ypixel = static_cast<int>(std::round(position.y() / model->getPixelSizeY()));

        // add the pixel the list of hit pixels
        PixelCharge::Pixel pixel(xpixel, ypixel);
        pixel_map[pixel].push_back(propagated_charge);

        LOG(DEBUG) << "set of " << propagated_charge.getCharge() << " propagated charges at "
                   << propagated_charge.getPosition() << " brought to pixel (" << pixel.x() << "," << pixel.y() << ")";
    }

    // create pixel charges
    LOG(INFO) << "Combining charges at same pixel";
    std::vector<PixelCharge> pixel_charges;
    for(auto& pixel : pixel_map) {
        unsigned int charge = 0;
        for(auto& propagated_charge : pixel.second) {
            charge += propagated_charge.getCharge();
        }
        pixel_charges.emplace_back(pixel.first, charge);

        LOG(DEBUG) << "set of " << charge << " charges at (" << pixel.first.x() << "," << pixel.first.y() << ")";
    }

    // dispatch message
    PixelChargeMessage pixel_message(pixel_charges, detector_);
    messenger_->dispatchMessage(pixel_message, "pixel");
}
