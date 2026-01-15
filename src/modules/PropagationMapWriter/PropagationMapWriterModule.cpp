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

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include <magic_enum/magic_enum.hpp>

#include "core/config/Configuration.hpp"
#include "core/geometry/Detector.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"
#include "core/utils/log.h"
#include "core/utils/unit.h"
#include "objects/DepositedCharge.hpp"
#include "objects/PixelCharge.hpp"
#include "objects/SensorCharge.hpp"
#include "tools/field_parser.h"

using namespace allpix;

PropagationMapWriterModule::PropagationMapWriterModule(Configuration& config,
                                                       Messenger* messenger,
                                                       std::shared_ptr<Detector> detector)
    : Module(config, detector), detector_(std::move(detector)), messenger_(messenger) {

    allow_multithreading();

    model_ = detector_->getModel();

    // This module requires both DepositedCharge and PixelCharge information
    messenger_->bindSingle<DepositedChargeMessage>(this, MsgFlags::REQUIRED);
    messenger_->bindSingle<PixelChargeMessage>(this, MsgFlags::REQUIRED);

    // Read number of bins
    const auto bins = config_.getArray<size_t>("bins");
    if(bins.size() != 3) {
        throw InvalidValueError(
            config_, "bins", "number of bins for three dimensions required, got values for " + std::to_string(bins.size()));
    }
    bins_ = {bins.at(0), bins.at(1), bins.at(2)};

    // Read field mapping from configuration
    field_mapping_ = config_.get<FieldMapping>("field_mapping");
    LOG(DEBUG) << "Propagation map will be generated for mapping " << magic_enum::enum_name(field_mapping_);

    // Select which carriers we look at
    carrier_type_ = config_.get<CarrierType>("carrier_type");
}

void PropagationMapWriterModule::initialize() {
    LOG(DEBUG) << "Setting up propagation map for detector " << detector_->getName();

    const auto pixel_size = model_->getPixelSize();
    const auto thickness = model_->getSensorSize().z();

    const auto sensor_max_z = model_->getSensorCenter().z() + (model_->getSensorSize().z() / 2.0);
    const auto sensor_min_z = model_->getSensorCenter().z() - (model_->getSensorSize().z() / 2.0);
    const auto thickness_domain = std::make_pair(sensor_min_z, sensor_max_z);

    // Calculate sizes from field mapping, starting from full pixel
    size_ = std::array<double, 3>({pixel_size.x(), pixel_size.y(), thickness});
    if(field_mapping_ == FieldMapping::PIXEL_HALF_LEFT || field_mapping_ == FieldMapping::PIXEL_HALF_RIGHT ||
       field_mapping_ == FieldMapping::PIXEL_QUADRANT_I || field_mapping_ == FieldMapping::PIXEL_QUADRANT_II ||
       field_mapping_ == FieldMapping::PIXEL_QUADRANT_III || field_mapping_ == FieldMapping::PIXEL_QUADRANT_IV) {
        size_[0] /= 2.0;
    }

    if(field_mapping_ == FieldMapping::PIXEL_HALF_TOP || field_mapping_ == FieldMapping::PIXEL_HALF_BOTTOM ||
       field_mapping_ == FieldMapping::PIXEL_QUADRANT_I || field_mapping_ == FieldMapping::PIXEL_QUADRANT_II ||
       field_mapping_ == FieldMapping::PIXEL_QUADRANT_III || field_mapping_ == FieldMapping::PIXEL_QUADRANT_IV) {
        size_[1] /= 2.0;
    }
    LOG(INFO) << "Using field with size " << Units::display(size_[0], "um") << "," << Units::display(size_[1], "um") << ","
              << Units::display(size_[2], "um");

    const auto scales = std::array<double, 2>({1., 1.});
    const auto offset = std::array<double, 2>({0., 0.});

    output_map_ =
        std::make_unique<PropagationMap>(model_, bins_, size_, FieldMapping::PIXEL_FULL, scales, offset, thickness_domain);
}

void PropagationMapWriterModule::run(Event* event) {

    auto deposits_message = messenger_->fetchMessage<DepositedChargeMessage>(this, event);
    auto pixel_message = messenger_->fetchMessage<PixelChargeMessage>(this, event);

    // Loop over all deposited charges:
    for(const auto& deposit : deposits_message->getData()) {

        // Ensure only one type is added to the map
        if(deposit.getType() != carrier_type_) {
            LOG(DEBUG) << "Skipping charge carriers (" << deposit.getType() << ") on "
                       << Units::display(deposit.getLocalPosition(), {"mm", "um"});
            continue;
        }

        // Fetch initial position and pixel index:
        const auto initial_position = deposit.getLocalPosition();
        const auto initial_charge = deposit.getCharge();
        const auto [xpixel, ypixel] = model_->getPixelIndex(initial_position);

        // Prepare lookup table:
        FieldTable probability_map{0};

        // Check if we find  matching a PixelCharge with this deposit in it:
        for(const auto& pixel_charge : pixel_message->getData()) {
            const auto propagated_charges = pixel_charge.find(&deposit);
            if(propagated_charges.empty()) {
                continue;
            }

            const auto final_index = pixel_charge.getIndex();

            std::int64_t final_charge = 0;
            for(const auto& prop_charge : propagated_charges) {
                final_charge += prop_charge->getCharge();
            }

            LOG(TRACE) << "Deposited charge at " << initial_position << " from pixel " << xpixel << "," << ypixel
                       << " ended on pixel " << final_index << ", relative: " << (final_index.x() - xpixel) << ","
                       << (final_index.y() - ypixel) << ", charge fraction "
                       << static_cast<double>(final_charge) / initial_charge;

            // Normalize the table entries by the initial charge of the deposit and add it to the map
            probability_map[FieldTable::getIndex(final_index.x() - xpixel, final_index.y() - ypixel)] =
                static_cast<double>(final_charge) / initial_charge;
        }

        // Add probability map to the target pixel map:
        output_map_->add(initial_position, probability_map);
    }
}

void PropagationMapWriterModule::finalize() {

    output_map_->checkField();

    // Fetch the field data form the output map and write it to a file:
    auto file_name = createOutputFile("test", "apf");

    // header info: carrier type
    const auto field_data =
        FieldData<double>("this is just a header that will be filled", bins_, size_, output_map_->getNormalizedField());

    auto writer = FieldWriter<double>(FieldQuantity::MAP);
    writer.writeFile(field_data, file_name, FileType::APF, "uniz");
}
