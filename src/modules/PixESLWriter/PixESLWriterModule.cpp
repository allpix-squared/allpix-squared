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

    // Get the parameters from the configuration file
    // config_.setDefault<double>("t_delay", Units::get(0, "ns"));

    warm_up_duration_ = config_.get<double>("warm_up_duration");
    BX_period_ = config_.get<double>("BX_period");
    mean_hit_rate_ = config_.get<double>("mean_hit_rate");
    peak_hit_rate_ = config_.get<double>("peak_hit_rate");
    peak_duration_ = config_.get<double>("peak_duration");

    if(peak_duration_ >= BX_period_) {
        throw InvalidValueError(config_, "peak_duration", "peak duration can't be greater than BX period");
    }

    // Set up file writer:
    writer_ = std::make_unique<apx::Writer>(output_file_,
                                            detector_->getName(),
                                            detector_->getType(),
                                            properties,
                                            number_of_events,
                                            "Allpix Squared",
                                            ALLPIX_PROJECT_VERSION,
                                            std::move(info));

    auto model = detector_->getModel();
    LOG(INFO) << "Matrix size x = " << model->getMatrixSize().x() << " mm";
    LOG(INFO) << "Matrix size y = " << model->getMatrixSize().y() << " mm";
    double matrix_area = model->getMatrixSize().x() * model->getMatrixSize().y();
    LOG(INFO) << "Matrix area = " << matrix_area << " mm²";

    lambda_warm_up = Units::convert(mean_hit_rate_, "/us/cm/cm") * Units::convert(matrix_area, "cm*cm") *
                     Units::convert(warm_up_duration_, "s");

    LOG(INFO) << "Mean hit rate = " << Units::convert(mean_hit_rate_, "/us/cm/cm") << " MHz/cm²";
    LOG(INFO) << "Warm-up duration = " << Units::convert(warm_up_duration_, "ns") << " ns";
    LOG(INFO) << "Lambda warm-up = " << lambda_warm_up << " events";
}

void PixESLWriterModule::run(Event* event) {

    LOG(INFO) << "This is the event " << event->number;
    // if isExist warm_up_duration.... {
    // Calculating the mean number of events for the warm-up duration, if set.
    if(event->number == 1) {
        allpix::poisson_distribution<int> mydist(lambda_warm_up);
        auto my_random_number = mydist(event->getRandomEngine());
        LOG(INFO) << "Mean number of events during warm-up = " << my_random_number << " events";
    }
    //}

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
