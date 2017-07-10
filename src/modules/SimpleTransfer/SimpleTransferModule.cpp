/**
 * @file
 * @brief Implementation of simple charge transfer module
 * @copyright MIT License
 */

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
#include "tools/ROOT.h"

#include "objects/PixelCharge.hpp"

using namespace allpix;

SimpleTransferModule::SimpleTransferModule(Configuration config, Messenger* messenger, std::shared_ptr<Detector> detector)
    : Module(config, detector), config_(std::move(config)), messenger_(messenger), detector_(std::move(detector)) {
    // Require propagated deposits for single detector
    messenger->bindSingle(this, &SimpleTransferModule::propagated_message_, MsgFlags::REQUIRED);
}

void SimpleTransferModule::run(unsigned int) {
    // Fetch detector model
    auto model = detector_->getModel();

    // Find corresponding pixels for all propagated charges
    LOG(TRACE) << "Transferring charges to pixels";
    unsigned int transferrred_charges_count = 0;
    std::map<PixelCharge::Pixel, std::vector<const PropagatedCharge*>, pixel_cmp> pixel_map;
    for(auto& propagated_charge : propagated_message_->getData()) {
        auto position = propagated_charge.getLocalPosition();
        // Ignore if outside depth range of implant
        // FIXME This logic should be improved
        if(std::fabs(position.z() - (model->getSensorCenter().z() + model->getSensorSize().z() / 2.0)) >
           config_.get<double>("max_depth_distance")) {
            LOG(DEBUG) << "Skipping set of " << propagated_charge.getCharge() << " propagated charges at "
                       << propagated_charge.getLocalPosition() << " because their local position is not in implant range";
            continue;
        }

        // Find the nearest pixel
        auto xpixel = static_cast<int>(std::round(position.x() / model->getPixelSize().x()));
        auto ypixel = static_cast<int>(std::round(position.y() / model->getPixelSize().y()));
        PixelCharge::Pixel pixel(xpixel, ypixel);

        // Ignore if out of pixel grid
        if(xpixel < 0 || xpixel >= model->getNPixels().x() || ypixel < 0 || ypixel >= model->getNPixels().y()) {
            LOG(DEBUG) << "Skipping set of " << propagated_charge.getCharge() << " propagated charges at "
                       << propagated_charge.getLocalPosition() << " because their nearest pixel " << pixel
                       << " is outside the grid";
            continue;
        }

        // Update statistics
        unique_pixels_.insert(pixel);
        transferrred_charges_count += propagated_charge.getCharge();

        LOG(DEBUG) << "Set of " << propagated_charge.getCharge() << " propagated charges at "
                   << propagated_charge.getLocalPosition() << " brought to pixel " << pixel;

        // Add the pixel the list of hit pixels
        pixel_map[pixel].emplace_back(&propagated_charge);
    }

    // Create pixel charges
    LOG(TRACE) << "Combining charges at same pixel";
    std::vector<PixelCharge> pixel_charges;
    for(auto& pixel : pixel_map) {
        unsigned int charge = 0;
        for(auto& propagated_charge : pixel.second) {
            charge += propagated_charge->getCharge();
        }
        pixel_charges.emplace_back(pixel.first, charge, pixel.second);

        LOG(DEBUG) << "Set of " << charge << " charges combined at " << pixel.first;
    }

    // Writing summary and update statistics
    LOG(INFO) << "Transferred " << transferrred_charges_count << " charges to " << pixel_map.size() << " pixels";
    total_transferrred_charges_ += transferrred_charges_count;

    // Dispatch message of pixel charges
    auto pixel_message = std::make_shared<PixelChargeMessage>(pixel_charges, detector_);
    messenger_->dispatchMessage(this, pixel_message);
}

void SimpleTransferModule::finalize() {
    // Print statistics
    LOG(INFO) << "Transferred total of " << total_transferrred_charges_ << " charges to " << unique_pixels_.size()
              << " different pixels";
}
