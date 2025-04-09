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
#include "PropagationMap.hpp"

#include <string>
#include <utility>

#include "core/utils/log.h"
#include "tools/field_parser.h"

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
    LOG(DEBUG) << "Setting up propagation map for detector " << detector_->getName();

    const auto pixel_size = model_->getPixelSize();
    const auto thickness = model_->getSensorSize().z();

    const auto sensor_max_z = model_->getSensorCenter().z() + model_->getSensorSize().z() / 2.0;
    const auto sensor_min_z = model_->getSensorCenter().z() - model_->getSensorSize().z() / 2.0;
    const auto thickness_domain = std::make_pair(sensor_min_z, sensor_max_z);

    size_ = std::array<double, 3>({pixel_size.x(), pixel_size.y(), thickness});
    bins_ = std::array<size_t, 3>({32, 32, 49});
    const auto scales = std::array<double, 2>({1., 1.});
    const auto offset = std::array<double, 2>({0., 0.});

    output_map_ =
        std::make_unique<PropagationMap>(model_, bins_, size_, FieldMapping::PIXEL_FULL, scales, offset, thickness_domain);
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

            // FIXME charge initial and charge final might be different, need to keep track of that!
            // at the very end we need to normalize!
            // in propagator we cannot use cumulative probability anymore I guess...

            // Add to map:
            output_map_->set(position, final, deposit->getCharge());

            LOG(TRACE) << "Deposited charge at " << deposit->getLocalPosition() << " from pixel " << xpixel << "," << ypixel
                       << " ended on pixel " << final << ", relative: " << (final.x() - xpixel) << ","
                       << (final.y() - ypixel);
        }
    }
}

void PropagationMapWriterModule::finalize() {

    // Fetch the field data form the output map and write it to a file:
    auto file_name = createOutputFile("test", "apf");

    const auto field_data =
        FieldData<double>("this is just a header that will be filled", bins_, size_, output_map_->get_field_data());

    auto writer = FieldWriter<double>(FieldQuantity::MAP);
    writer.writeFile(field_data, file_name, FileType::APF, "uniz");
}
