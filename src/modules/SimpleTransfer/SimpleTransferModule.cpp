#include "SimpleTransferModule.hpp"

#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

#include "core/config/exceptions.h"
#include "core/geometry/HybridPixelDetectorModel.hpp"
#include "core/utils/log.h"
#include "core/utils/unit.h"

#include "objects/PixelCharge.hpp"

using namespace allpix;

// constructor to load the module
SimpleTransferModule::SimpleTransferModule(Configuration config, Messenger* messenger, std::shared_ptr<Detector> detector)
    : Module(config, detector), config_(std::move(config)), messenger_(messenger), detector_(std::move(detector)) {
    // require propagated deposits for single detector
    messenger->bindSingle(this, &SimpleTransferModule::propagated_message_, MsgFlags::REQUIRED);
}

// run method that does the main computations for the module
void SimpleTransferModule::run(unsigned int) {
    // get detector model
    auto model = std::dynamic_pointer_cast<HybridPixelDetectorModel>(detector_->getModel());
    if(model == nullptr) {
        throw ModuleError("Detector model of " + detector_->getName() +
                          " is not a PixelDetectorModel: other models are not supported by this module!");
    }

    // find pixels for all propagated charges
    LOG(TRACE) << "Transferring charges to pixels";
    unsigned int transferrred_charges_count = 0;
    std::map<PixelCharge::Pixel, std::vector<PropagatedCharge>, pixel_cmp> pixel_map;
    for(auto& propagated_charge : propagated_message_->getData()) {
        auto position = propagated_charge.getPosition();
        // ignore if outside z range of implant
        // FIXME: this logic should be extended
        if(std::fabs(position.z() - (model->getSensorCenter().z() + model->getSensorSize().z() / 2.0)) >
           config_.get<double>("max_depth_distance")) {
            LOG(DEBUG) << "Skipping set of " << propagated_charge.getCharge() << " propagated charges at "
                       << propagated_charge.getPosition() << " because their local position is not in implant range";
            continue;
        }

        // find the nearest pixel
        auto xpixel = static_cast<int>(std::round(position.x() / model->getPixelSize().x()));
        auto ypixel = static_cast<int>(std::round(position.y() / model->getPixelSize().y()));
        PixelCharge::Pixel pixel(xpixel, ypixel);

        // ignore if out of pixel grid
        if(xpixel < 0 || xpixel >= model->getNPixels().x() || ypixel < 0 || ypixel >= model->getNPixels().y()) {
            LOG(DEBUG) << "Skipping set of " << propagated_charge.getCharge() << " propagated charges at "
                       << propagated_charge.getPosition() << " because their nearest pixel (" << pixel.x() << ","
                       << pixel.y() << ") is outside the grid";
            continue;
        }

        // update statistics
        unique_pixels_.insert(pixel);
        transferrred_charges_count += propagated_charge.getCharge();

        LOG(DEBUG) << "Set of " << propagated_charge.getCharge() << " propagated charges at "
                   << propagated_charge.getPosition() << " brought to pixel (" << pixel.x() << "," << pixel.y() << ")";

        // add the pixel the list of hit pixels
        pixel_map[pixel].emplace_back(propagated_charge);
    }

    // create pixel charges
    LOG(TRACE) << "Combining charges at same pixel";
    std::vector<PixelCharge> pixel_charges;
    for(auto& pixel : pixel_map) {
        unsigned int charge = 0;
        for(auto& propagated_charge : pixel.second) {
            charge += propagated_charge.getCharge();
        }
        pixel_charges.emplace_back(pixel.first, charge);

        LOG(DEBUG) << "Set of " << charge << " charges combined at (" << pixel.first.x() << "," << pixel.first.y() << ")";
    }

    // writing summary and update statistics
    LOG(INFO) << "Transferred " << transferrred_charges_count << " charges to " << pixel_map.size() << " pixels";
    total_transferrred_charges_ += transferrred_charges_count;

    // dispatch message
    auto pixel_message = std::make_shared<PixelChargeMessage>(pixel_charges, detector_);
    messenger_->dispatchMessage(this, pixel_message);
}

// print statistics
void SimpleTransferModule::finalize() {
    LOG(INFO) << "Transferred total of " << total_transferrred_charges_ << " charges to " << unique_pixels_.size()
              << " different pixels";
}
