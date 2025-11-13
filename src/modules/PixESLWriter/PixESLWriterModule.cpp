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
    : SequentialModule(config, detector), detector_(std::move(detector)), messenger_(messenger), timestamp(0) {

    // This is a sequential module and therefore thread-safe
    allow_multithreading();

    // Messages: register this module with the central messenger to request a certaintype of input messages:
    messenger_->bindSingle<PixelHitMessage>(this, MsgFlags::REQUIRED);
}

void PixESLWriterModule::initialize() {

    output_file_ = createOutputFile(config_.get<std::string>("file_name"), "apx", false);

    output_plots_ = config_.get<bool>("output_plots");

    // Collect information about this simulation:
    auto& global_config = getConfigManager()->getGlobalConfiguration();
    std::string info = "Simulation from config " + global_config.getFilePath().string() + " with random seed " +
                       std::to_string(global_config.get<uint64_t>("random_seed_core"));
    const auto number_of_events = global_config.get<uint64_t>("number_of_events");

    // Calculate the active matrix area
    auto model = detector_->getModel();
    LOG(DEBUG) << "Active matrix length x : " << model->getMatrixSize().x() << " mm";
    LOG(DEBUG) << "Active matrix length y : " << model->getMatrixSize().y() << " mm";
    double matrix_area = model->getMatrixSize().x() * model->getMatrixSize().y();
    LOG(INFO) << "Active matrix area : " << matrix_area << " mm^2";

    // Calculate the exponential distribution parameter based on the given hit rate
    mean_hit_rate_ = config_.get<double>("mean_hit_rate", (Units::get("/ns/mm/mm")));
    lambda_mean_rate = mean_hit_rate_ * matrix_area;
    tau_mean_rate = 1 / (Units::convert(lambda_mean_rate, "ns"));

    if(config_.has("BX_period")) {
        BX_period_ = config_.get<double>("BX_period", (Units::get("ns")));
        LOG(INFO) << "Mean hit rate on the active matrix : " << lambda_mean_rate * BX_period_ << " events per BX";
        // Set up properties:
        properties = {"column", "row", "charge", "timestamp", "BXID"};
    } else {
        LOG(INFO) << "Mean hit rate on the active matrix : " << lambda_mean_rate << " events per ns";
        // Set up properties:
        properties = {"column", "row", "charge", "timestamp"};
    }

    LOG(INFO) << "Mean time between 2 events : " << tau_mean_rate << " ns";

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
                                                     static_cast<double>(Units::convert(tau_mean_rate, "ns")) * 10);
    }
}

void PixESLWriterModule::run(Event* event) {

    // Generate the distribution to associate a timestamp for event containing a PixelHit message:
    allpix::exponential_distribution<double> time_dist(lambda_mean_rate);
    auto dt = time_dist(event->getRandomEngine());
    timestamp += dt;
    LOG(INFO) << "Time between this event and the previous : " << dt << " ns";
    LOG(INFO) << "Timestamp : " << timestamp << " ns";

    if(output_plots_) {
        time_between_events_->Fill(dt);
    }

    // Calculate the BXID depending on the timestamp:
    if(config_.has("BX_period")) {
        BX_id = static_cast<int>(timestamp.load() / BX_period_);
        LOG(INFO) << "BX " << BX_id;
    }

    // Generate new event for output:
    auto apx_event = writer_->createEvent(event->number);

    // Fetch the messages for this event from the messenger:
    auto message = messenger_->fetchMessage<PixelHitMessage>(this, event);

    // Loop over hits and write event record
    for(const auto& hit : message->getData()) {
        const auto index = hit.getIndex();
        (config_.has("BX_period"))
            ? apx_event.appendRecord({index.x(), index.y(), hit.getSignal(), timestamp + hit.getGlobalTime(), BX_id})
            : apx_event.appendRecord({index.x(), index.y(), hit.getSignal(), timestamp + hit.getGlobalTime()});
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
