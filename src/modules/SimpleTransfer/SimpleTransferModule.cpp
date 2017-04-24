#include "SimpleTransferModule.hpp"

#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

#include "core/config/exceptions.h"
#include "core/geometry/PixelDetectorModel.hpp"
#include "core/utils/log.h"
#include "core/utils/unit.h"

#include "objects/PixelCharge.hpp"

using namespace allpix;

// constructor to load the module
SimpleTransferModule::SimpleTransferModule(Configuration config, Messenger* messenger, std::shared_ptr<Detector> detector)
    : Module(detector), config_(std::move(config)), messenger_(messenger), detector_(std::move(detector)),
      propagated_message_() {
    // require propagated deposits for single detector
    messenger->bindSingle(this, &SimpleTransferModule::propagated_message_, MsgFlags::REQUIRED);
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
    // get detector model
    auto model = std::dynamic_pointer_cast<PixelDetectorModel>(detector_->getModel());
    if(model == nullptr) {
        throw ModuleError("Detector model of " + detector_->getName() +
                          " is not a PixelDetectorModel: other models are not supported by this module!");
    }

    // find pixels for all propagated charges
    LOG(INFO) << "Transferring charges to pixels";
    std::map<PixelCharge::Pixel, std::vector<PropagatedCharge>, pixel_cmp> pixel_map;
    for(auto& propagated_charge : propagated_message_->getData()) {
        auto position = propagated_charge.getPosition();
        // ignore if outside z range of implant
        // FIXME: this logic should be extended
        if(std::fabs(position.z() - model->getSensorMinZ()) > config_.get<double>("max_depth_distance")) {
            LOG(DEBUG) << "skipping set of " << propagated_charge.getCharge() << " propagated charges at "
                       << propagated_charge.getPosition() << " because their local position is not in implant range";
            continue;
        }

        // find the nearest pixel
        auto xpixel = static_cast<int>(std::round(position.x() / model->getPixelSizeX()));
        auto ypixel = static_cast<int>(std::round(position.y() / model->getPixelSizeY()));
        PixelCharge::Pixel pixel(xpixel, ypixel);

        // ignore if out of pixel grid
        if(xpixel < 0 || xpixel >= model->getNPixelsX() || ypixel < 0 || ypixel >= model->getNPixelsY()) {
            LOG(DEBUG) << "skipping set of " << propagated_charge.getCharge() << " propagated charges at "
                       << propagated_charge.getPosition() << " because their nearest pixel (" << pixel.x() << ","
                       << pixel.y() << ") is outside the grid";
            continue;
        }

        // add the pixel the list of hit pixels
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
