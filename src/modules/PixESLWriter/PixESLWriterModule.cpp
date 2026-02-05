/**
 * @file
 * @brief Implementation of PixESLWriter module
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "PixESLWriterModule.hpp"

#include <memory>
#include <string>
#include <utility>

#include <libapx/writer.hpp>

#include "core/utils/log.h"

using namespace allpix;

PixESLWriterModule::PixESLWriterModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector)
    : SequentialModule(config, detector), detector_(std::move(detector)), messenger_(messenger) {

    // This is a sequential module and therefore thread-safe
    allow_multithreading();

    // Messages: register this module with the central messenger to request a certaintype of input messages:
    messenger_->bindSingle<PixelHitMessage>(this, MsgFlags::REQUIRED);
}

void PixESLWriterModule::initialize() {

    // Set up properties:
    const std::vector<std::string> properties{"column", "row", "charge", "toa"};

    output_file_ = createOutputFile(config_.get<std::string>("file_name"), "apx", false);

    // Collect information about this simulation:
    auto& global_config = getConfigManager()->getGlobalConfiguration();
    std::string info = "Simulation from config " + global_config.getFilePath().string() + " with random seed " +
                       std::to_string(global_config.get<uint64_t>("random_seed_core"));
    const auto number_of_events = global_config.get<uint64_t>("number_of_events");

    // Set up file writer:
    writer_ = std::make_unique<apx::Writer>(output_file_,
                                            detector_->getName(),
                                            detector_->getType(),
                                            properties,
                                            number_of_events,
                                            "Allpix Squared",
                                            ALLPIX_PROJECT_VERSION,
                                            std::move(info));
}

void PixESLWriterModule::run(Event* event) {

    // Generate new event for output:
    auto apx_event = writer_->createEvent(event->number);

    // Fetch the messages for this event from the messenger:
    auto message = messenger_->fetchMessage<PixelHitMessage>(this, event);

    // Loop over hits and write event record
    for(const auto& hit : message->getData()) {
        const auto index = hit.getIndex();
        apx_event.appendRecord({index.x(), index.y(), hit.getSignal(), hit.getGlobalTime()});
    }

    LOG(TRACE) << "Event " << apx_event.getID() << " has " << apx_event.getRecords().size() << " records";

    // Stream the event to file
    (*writer_) << apx_event;
}

void PixESLWriterModule::finalize() {
    // Print statistics
    LOG(STATUS) << "Wrote " << writer_->getRecordCount() << " records in " << writer_->getEventCount()
                << " events to file:\n"
                << output_file_;
}
