/**
 * @file
 * @brief Implementation of database writer module
 *
 * @copyright Copyright (c) 2018-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "DatabaseWriterModule.hpp"

#include <fstream>
#include <optional>
#include <string>
#include <utility>

#include <TBranchElement.h>
#include <TClass.h>

#include "core/config/ConfigReader.hpp"
#include "core/utils/log.h"
#include "core/utils/type.h"

#include "objects/Object.hpp"
#include "objects/objects.h"

using namespace allpix;

thread_local std::shared_ptr<pqxx::connection> DatabaseWriterModule::conn_ = nullptr;

DatabaseWriterModule::DatabaseWriterModule(Configuration& config, Messenger* messenger, GeometryManager*)
    : SequentialModule(config), messenger_(messenger) {
    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();
    // Bind to all messages with filter
    messenger_->registerFilter(this, &DatabaseWriterModule::filter);

    config_.setDefault("global_timing", false);
    config_.setDefault("require_sequence", false);

    config_.setDefault("run_id", "none");

    // retrieving configuration parameters
    host_ = config_.get<std::string>("host");
    port_ = config_.get<std::string>("port");
    database_name_ = config_.get<std::string>("database_name");
    user_ = config_.get<std::string>("user");
    password_ = config_.get<std::string>("password");
    run_id_ = config_.get<std::string>("run_id");

    // Select pixel hit timing information to be saved:
    timing_global_ = config_.get<bool>("global_timing");

    // Waive sequence requirement if requested by user
    if(!config_.get<bool>("require_sequence")) {
        waive_sequence_requirement();
    }
}

void DatabaseWriterModule::prepare_statements(const std::shared_ptr<pqxx::connection>& connection) {
    LOG(DEBUG) << "Preparing database statements";
    connection->prepare("add_run", "INSERT INTO Run (run_id) VALUES ($1) RETURNING run_nr;");

    connection->prepare("add_event", "INSERT INTO Event (run_nr, eventID) VALUES ($1, $2) RETURNING event_nr;");

    connection->prepare(
        "add_mctrack",
        "INSERT INTO MCTrack (run_nr, event_nr, detector, address, parentAddress, particleID, "
        "productionProcess, productionVolume, initialPositionX, initialPositionY, initialPositionZ, "
        "finalPositionX, finalPositionY, finalPositionZ, initialTime, finalTime, initialKineticEnergy, finalKineticEnergy) "
        "VALUES ("
        "$1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17, $18) RETURNING mctrack_nr;");

    connection->prepare("add_mcparticle",
                        "INSERT INTO MCParticle (run_nr, event_nr, mctrack_nr, detector, address, parentAddress, "
                        "trackAddress, particleID, localStartPointX, localStartPointY, localStartPointZ, localEndPointX, "
                        "localEndPointY, localEndPointZ, globalStartPointX, globalStartPointY, globalStartPointZ, "
                        "globalEndPointX, globalEndPointY, globalEndPointZ) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, "
                        "$10, $11, $12, $13, $14, $15, $16, $17, $18, $19, $20) RETURNING mcparticle_nr;");

    connection->prepare("add_depositedcharge",
                        "INSERT INTO DepositedCharge (run_nr, event_nr, mcparticle_nr, detector, carriertype, charge, "
                        "localx, localy, localz, globalx, globaly, globalz) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, "
                        "$10, $11, $12) RETURNING depositedcharge_nr;");

    connection->prepare("add_propagatedcharge",
                        "INSERT INTO PropagatedCharge (run_nr, event_nr, depositedcharge_nr, detector, carriertype, charge, "
                        "localx, localy, localz, globalx, globaly, globalz) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, "
                        "$10, $11, $12) RETURNING propagatedcharge_nr;");

    connection->prepare(
        "add_pixelcharge",
        "INSERT INTO PixelCharge (run_nr, event_nr, propagatedcharge_nr, detector, charge, x, y, localx, localy, globalx, "
        "globaly) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11) RETURNING pixelCharge_nr");

    connection->prepare("add_pixelhit",
                        "INSERT INTO PixelHit (run_nr, event_nr, mcparticle_nr, pixelcharge_nr, detector, x, y, signal, "
                        "hittime) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9) RETURNING pixelHit_nr;");
}

void DatabaseWriterModule::initializeThread() {
    // Establishing connection to the database
    conn_ = std::make_shared<pqxx::connection>("host=" + host_ + " port=" + port_ + " dbname=" + database_name_ +
                                               " user=" + user_ + " password=" + password_);
    if(!conn_->is_open()) {
        throw ModuleError("Could not connect to database " + database_name_ + " at host " + host_);
    }

    prepare_statements(conn_);

    // Inserting run entry in the database
    try {
        // Open new transaction
        pqxx::work transaction(*conn_);
        auto runR = transaction.exec_prepared1("add_run", run_id_);
        run_nr_ = runR.front().as<int>();
        // Commit transaction to database:
        transaction.commit();

    } catch(const std::exception& e) {
        throw ModuleError("SQL error: " + std::string(e.what()));
    }

    // Read include and exclude list
    if(config_.has("include") && config_.has("exclude")) {
        throw InvalidValueError(config_, "exclude", "include and exclude parameter are mutually exclusive");
    } else if(config_.has("include")) {
        auto inc_arr = config_.getArray<std::string>("include");
        include_.insert(inc_arr.begin(), inc_arr.end());
    } else if(config_.has("exclude")) {
        auto exc_arr = config_.getArray<std::string>("exclude");
        exclude_.insert(exc_arr.begin(), exc_arr.end());
    }
}

bool DatabaseWriterModule::filter(const std::shared_ptr<BaseMessage>& message,
                                  const std::string& message_name) const { // NOLINT
    try {
        const BaseMessage* inst = message.get();
        std::string name_str = " without a name";
        if(!message_name.empty()) {
            name_str = " named " + message_name;
        }
        LOG(TRACE) << "Database writer received " << allpix::demangle(typeid(*inst).name()) << name_str;

        // Read the object
        auto object_array = message->getObjectArray();
        if(object_array.empty()) {
            return false;
        }
        const Object& first_object = object_array[0];
        std::string class_name = allpix::demangle(typeid(first_object).name());

        // Check if this message should be kept
        if((!include_.empty() && include_.find(class_name) == include_.end()) ||
           (!exclude_.empty() && exclude_.find(class_name) != exclude_.end())) {
            LOG(TRACE) << "Database writer ignored message with object " << allpix::demangle(typeid(*inst).name())
                       << " because it has been excluded or not explicitly included";
            return false;
        }
    } catch(MessageWithoutObjectException& e) {
        const BaseMessage* inst = message.get();
        LOG(WARNING) << "Database writer cannot process message of type" << allpix::demangle(typeid(*inst).name())
                     << " with name " << message_name;
        return false;
    }

    return true;
}

void DatabaseWriterModule::run(Event* event) {
    auto messages = messenger_->fetchFilteredMessages(this, event);

    // TO BE NOTED
    // the correct relations of objects in the database are guaranteed by the fact that sequence of dispatched messages
    // within one event always follows this order: MCTrack -> MCParticle -> DepositedCharge -> PropagatedCharge ->
    // PixelCharge -> PixelHit

    // Using optionals to only write values to database if the respective references have been set by other messages stored
    std::optional<int> mctrack_nr;
    std::optional<int> mcparticle_nr;
    std::optional<int> depositedcharge_nr;
    std::optional<int> propagatedcharge_nr;
    std::optional<int> pixelcharge_nr;

    LOG(TRACE) << "Writing new objects to database";
    try {
        // Open new transaction
        pqxx::work transaction(*conn_);
        LOG(DEBUG) << "Started new database transaction";

        // Writing entry to event table
        auto event_row = transaction.exec_prepared1("add_event", run_nr_, event->number);
        int event_nr = event_row.front().as<int>();

        // Looping through messages
        for(auto& pair : messages) {
            auto& message = pair.first;
            auto detectorName = (message->getDetector() != nullptr ? message->getDetector()->getName() : "global");

            for(const auto& object : message->getObjectArray()) {
                auto& o = object.get();
                std::string class_name = allpix::demangle(typeid(o).name());

                // Writing objects to corresponding database tables
                if(class_name == "PixelHit") {
                    auto hit = static_cast<PixelHit&>(object.get());
                    auto hit_row = transaction.exec_prepared1("add_pixelhit",
                                                              run_nr_,
                                                              event_nr,
                                                              mcparticle_nr,
                                                              pixelcharge_nr,
                                                              detectorName,
                                                              hit.getIndex().X(),
                                                              hit.getIndex().Y(),
                                                              hit.getSignal(),
                                                              (timing_global_ ? hit.getGlobalTime() : hit.getLocalTime()));
                    LOG(TRACE) << "Inserted PixelHit with db id " << hit_row.front().as<int>();
                } else if(class_name == "PixelCharge") {
                    auto charge = static_cast<PixelCharge&>(object.get());
                    auto charge_row = transaction.exec_prepared1("add_pixelcharge",
                                                                 run_nr_,
                                                                 event_nr,
                                                                 propagatedcharge_nr,
                                                                 detectorName,
                                                                 charge.getCharge(),
                                                                 charge.getIndex().X(),
                                                                 charge.getIndex().Y(),
                                                                 charge.getPixel().getLocalCenter().X(),
                                                                 charge.getPixel().getLocalCenter().Y(),
                                                                 charge.getPixel().getGlobalCenter().X(),
                                                                 charge.getPixel().getGlobalCenter().Y());
                    pixelcharge_nr = charge_row.front().as<int>();
                    LOG(TRACE) << "Inserted PixelCharge  with db id " << pixelcharge_nr.value();
                } else if(class_name == "PropagatedCharge") {
                    PropagatedCharge charge = static_cast<PropagatedCharge&>(object.get());
                    auto prop_row = transaction.exec_prepared1("add_propagatedcharge",
                                                               run_nr_,
                                                               event_nr,
                                                               depositedcharge_nr,
                                                               detectorName,
                                                               static_cast<int>(charge.getType()),
                                                               charge.getCharge(),
                                                               charge.getLocalPosition().X(),
                                                               charge.getLocalPosition().Y(),
                                                               charge.getLocalPosition().Z(),
                                                               charge.getGlobalPosition().X(),
                                                               charge.getGlobalPosition().Y(),
                                                               charge.getGlobalPosition().Z());
                    propagatedcharge_nr = prop_row.front().as<int>();
                    LOG(TRACE) << "Inserted PropagatedCharge with db id " << propagatedcharge_nr.value();
                } else if(class_name == "MCTrack") {
                    auto track = static_cast<MCTrack&>(object.get());
                    auto mctrack_row = transaction.exec_prepared1("add_mctrack",
                                                                  run_nr_,
                                                                  event_nr,
                                                                  detectorName,
                                                                  reinterpret_cast<uintptr_t>(&object),           // NOLINT
                                                                  reinterpret_cast<uintptr_t>(track.getParent()), // NOLINT
                                                                  track.getParticleID(),
                                                                  track.getCreationProcessName(),
                                                                  track.getOriginatingVolumeName(),
                                                                  track.getStartPoint().X(),
                                                                  track.getStartPoint().Y(),
                                                                  track.getStartPoint().Z(),
                                                                  track.getEndPoint().X(),
                                                                  track.getEndPoint().Y(),
                                                                  track.getEndPoint().Z(),
                                                                  track.getGlobalStartTime(),
                                                                  track.getGlobalEndTime(),
                                                                  track.getKineticEnergyInitial(),
                                                                  track.getKineticEnergyFinal());
                    mctrack_nr = mctrack_row.front().as<int>();
                    LOG(TRACE) << "Inserted MCTrack with db id " << mctrack_nr.value();
                } else if(class_name == "DepositedCharge") {
                    auto charge = static_cast<DepositedCharge&>(object.get());
                    auto depos_row = transaction.exec_prepared1("add_depositedcharge",
                                                                run_nr_,
                                                                event_nr,
                                                                mcparticle_nr,
                                                                detectorName,
                                                                static_cast<int>(charge.getType()),
                                                                charge.getCharge(),
                                                                charge.getLocalPosition().X(),
                                                                charge.getLocalPosition().Y(),
                                                                charge.getLocalPosition().Z(),
                                                                charge.getGlobalPosition().X(),
                                                                charge.getGlobalPosition().Y(),
                                                                charge.getGlobalPosition().Z());
                    depositedcharge_nr = depos_row.front().as<int>();
                    LOG(TRACE) << "Inserted DepositedCharge with db id " << depositedcharge_nr.value();
                } else if(class_name == "MCParticle") {
                    auto particle = static_cast<MCParticle&>(object.get());
                    auto mcp_row = transaction.exec_prepared1("add_mcparticle",
                                                              run_nr_,
                                                              event_nr,
                                                              mctrack_nr,
                                                              detectorName,
                                                              reinterpret_cast<uintptr_t>(&object),              // NOLINT
                                                              reinterpret_cast<uintptr_t>(particle.getParent()), // NOLINT
                                                              reinterpret_cast<uintptr_t>(particle.getTrack()),  // NOLINT
                                                              particle.getParticleID(),
                                                              particle.getLocalStartPoint().X(),
                                                              particle.getLocalStartPoint().Y(),
                                                              particle.getLocalStartPoint().Z(),
                                                              particle.getLocalEndPoint().X(),
                                                              particle.getLocalEndPoint().Y(),
                                                              particle.getLocalEndPoint().Z(),
                                                              particle.getGlobalStartPoint().X(),
                                                              particle.getGlobalStartPoint().Y(),
                                                              particle.getGlobalStartPoint().Z(),
                                                              particle.getGlobalEndPoint().X(),
                                                              particle.getGlobalEndPoint().Y(),
                                                              particle.getGlobalEndPoint().Z());
                    mcparticle_nr = mcp_row.front().as<int>();
                    LOG(TRACE) << "Inserted MCParticle with db id " << mcparticle_nr.value();
                } else {
                    LOG(WARNING) << "Following object type is not yet accounted for in database output: " << class_name;
                }
                write_cnt_++;
            }
            msg_cnt_++;
        }

        // Commit transaction to database:
        transaction.commit();
        LOG(DEBUG) << "Database transaction completed";
    } catch(const std::exception& e) {
        throw ModuleError("SQL error: " + std::string(e.what()));
    }
}

void DatabaseWriterModule::finalizeThread() {
// Disconnecting from database
#if PQXX_VERSION_MAJOR > 6
    conn_->close();
#else
    conn_->disconnect();
#endif
}

void DatabaseWriterModule::finalize() {
    LOG(STATUS) << "Wrote " << write_cnt_ << " objects from " << msg_cnt_ << " messages to database" << std::endl;
}
