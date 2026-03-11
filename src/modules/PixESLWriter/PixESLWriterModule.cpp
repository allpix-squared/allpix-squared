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

#include "core/utils/distributions.h"
#include "core/utils/log.h"

using namespace allpix;

PixESLWriterModule::PixESLWriterModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector)
    : SequentialModule(config, detector), detector_(std::move(detector)), messenger_(messenger) {

    // This is a sequential module and therefore thread-safe
    allow_multithreading();

    message_pixelcharge_ = config_.get<bool>("write_pixelcharge", false);
    if(message_pixelcharge_) {
        messenger_->bindSingle<PixelChargeMessage>(this, MsgFlags::REQUIRED);
    } else {
        messenger_->bindSingle<PixelHitMessage>(this, MsgFlags::REQUIRED);
    }
}

void PixESLWriterModule::initialize() {

    // Set up properties:
    std::vector<std::string> properties{"column", "row", "charge", "timestamp"};

    output_file_ = createOutputFile(config_.get<std::string>("file_name"), "apx", false);
    output_plots_ = config_.get<bool>("output_plots");

    // Collect information about this simulation:
    auto& global_config = getConfigManager()->getGlobalConfiguration();
    std::string info = "Simulation from config " + global_config.getFilePath().string() + " with random seed " +
                       std::to_string(global_config.get<uint64_t>("random_seed_core"));
    const auto number_of_events = global_config.get<uint64_t>("number_of_events");

    // Calculate the active matrix area
    auto model = detector_->getModel();
    LOG(DEBUG) << "Calculating rate for active matrix size of " << Units::display(model->getMatrixSize(), {"mm"});
    const auto matrix_area = model->getMatrixSize().x() * model->getMatrixSize().y();
    LOG(INFO) << "Active matrix area : " << matrix_area << " mm^2";

    // Calculate the exponential distribution parameter based on the given hit rate
    const auto mean_hit_rate = config_.get<double>("mean_hit_rate");
    lambda_mean_rate_ = mean_hit_rate * matrix_area;

    if(config_.has("bx_period")) {
        bx_period_ = config_.get<double>("bx_period");
        LOG(INFO) << "Mean hit rate on the active matrix: " << lambda_mean_rate_ * bx_period_.value()
                  << " events per bunch crossing";
        properties.emplace_back("bx_id");
    } else {
        LOG(INFO) << "Mean hit rate on the active matrix: " << lambda_mean_rate_ << " events per ns";
    }

    LOG(INFO) << "Mean time between consecutive events: " << Units::display(1. / lambda_mean_rate_, {"ns", "us"});

    // Set up file writer:
    writer_ = std::make_unique<apx::Writer>(output_file_,
                                            detector_->getName(),
                                            detector_->getType(),
                                            properties,
                                            number_of_events,
                                            "Allpix Squared",
                                            ALLPIX_PROJECT_VERSION,
                                            std::move(info));

    if(output_plots_) {
        time_between_events_ = CreateHistogram<TH1D>("time_between_events",
                                                     "Interval time between two events;Interval time [ns]; number of events",
                                                     100,
                                                     0,
                                                     8. / lambda_mean_rate_);
    }
}

template <> void PixESLWriterModule::transfer_data<PixelHitMessage>(Event* in, apx::Event& out, std::int64_t bx) {
    // Fetch the messages for this event from the messenger:
    auto message = messenger_->fetchMessage<PixelHitMessage>(this, in);

    // Loop over hits and write event record
    for(const auto& hit : message->getData()) {
        const auto index = hit.getIndex();
        (bx_period_.has_value())
            ? out.appendRecord({index.x(), index.y(), hit.getSignal(), timestamp_ + hit.getGlobalTime(), bx})
            : out.appendRecord({index.x(), index.y(), hit.getSignal(), timestamp_ + hit.getGlobalTime()});
    }
}

template <> void PixESLWriterModule::transfer_data<PixelChargeMessage>(Event* in, apx::Event& out, std::int64_t bx) {
    // Fetch the messages for this event from the messenger:
    auto message = messenger_->fetchMessage<PixelChargeMessage>(this, in);

    // Loop over pixel charges and write event record
    for(const auto& pc : message->getData()) {
        const auto index = pc.getIndex();
        (bx_period_.has_value())
            ? out.appendRecord({index.x(), index.y(), pc.getCharge(), timestamp_ + pc.getGlobalTime(), bx})
            : out.appendRecord({index.x(), index.y(), pc.getCharge(), timestamp_ + pc.getGlobalTime()});
    }
}

void PixESLWriterModule::run(Event* event) {

    // Generate the distribution to associate a timestamp for event containing a PixelHit message:
    const allpix::exponential_distribution<double> time_dist(lambda_mean_rate_);
    auto dt = time_dist(event->getRandomEngine());
    timestamp_ += dt;
    LOG(INFO) << "Time since previous event: " << Units::display(dt, {"ns"})
              << ", timestamp: " << Units::display(timestamp_.load(), {"ns"});

    if(output_plots_) {
        time_between_events_->Fill(dt);
    }

    // Calculate the BXID depending on the timestamp:
    std::int64_t current_bx_id = 0;
    if(bx_period_.has_value()) {
        current_bx_id = static_cast<std::int64_t>(timestamp_.load() / bx_period_.value());
        LOG(INFO) << "BX " << current_bx_id;
    }

    // Generate new event for output:
    auto apx_event = writer_->createEvent(event->number);

    // Append the data from the message:
    if(message_pixelcharge_) {
        transfer_data<PixelChargeMessage>(event, apx_event, current_bx_id);
    } else {
        transfer_data<PixelHitMessage>(event, apx_event, current_bx_id);
    }

    LOG(TRACE) << "Event " << apx_event.getID() << " has " << apx_event.getRecords().size() << " records";

    // Stream the event to file
    (*writer_) << apx_event;
}

void PixESLWriterModule::finalize() {

    if(output_plots_) {
        time_between_events_->Write();
    }

    // Print statistics
    LOG(STATUS) << "Wrote " << writer_->getRecordCount() << " records in " << writer_->getEventCount()
                << " events to file:\n"
                << output_file_;
}
