/**
 * @file
 * @brief Implementation of database writer module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "DatabaseWriterModule.hpp"

#include <fstream>
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
thread_local std::shared_ptr<pqxx::nontransaction> DatabaseWriterModule::W_ = nullptr;

DatabaseWriterModule::DatabaseWriterModule(Configuration& config, Messenger* messenger, GeometryManager*)
    : SequentialModule(config), messenger_(messenger) {
    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();
    // Bind to all messages with filter
    messenger_->registerFilter(this, &DatabaseWriterModule::filter);

    config_.setDefault("global_timing", false);
    config_.setDefault("waive_sequence_requirement", false);

    config_.setDefault("run_id", "none");
}

void DatabaseWriterModule::initialize() {

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
    if(config_.get<bool>("waive_sequence_requirement")) {
        waive_sequence_requirement();
    }
}

void DatabaseWriterModule::prepare_statements(std::shared_ptr<pqxx::connection> connection) {
    LOG(DEBUG) << "Preparing database statements";
    connection->prepare("add_event", "INSERT INTO Event (run_nr, eventID) VALUES ($1, $2) RETURNING event_nr;");

    connection->prepare("add_mctrack",
                        "INSERT INTO MCTrack (run_nr, event_nr, detector, address, parentAddress, particleID, "
                        "productionProcess, productionVolume, initialPositionX, initialPositionY, initialPositionZ, "
                        "finalPositionX, finalPositionY, finalPositionZ, initialKineticEnergy, finalKineticEnergy) VALUES ("
                        "$1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16) RETURNING mctrack_nr;");

    connection->prepare("add_mcparticle_with_ref",
                        "INSERT INTO MCParticle (run_nr, event_nr, mctrack_nr, detector, address, parentAddress, "
                        "trackAddress, particleID, localStartPointX, localStartPointY, localStartPointZ, localEndPointX, "
                        "localEndPointY, localEndPointZ, globalStartPointX, globalStartPointY, globalStartPointZ, "
                        "globalEndPointX, globalEndPointY, globalEndPointZ) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, "
                        "$10, $11, $12, $13, $14, $15, $16, $17, $18, $19, $20) RETURNING mcparticle_nr;");
    connection->prepare(
        "add_mcparticle",
        "INSERT INTO MCParticle (run_nr, event_nr, detector, address, parentAddress, trackAddress, particleID, "
        "localStartPointX, localStartPointY, localStartPointZ, localEndPointX, localEndPointY, localEndPointZ, "
        "globalStartPointX, globalStartPointY, globalStartPointZ, globalEndPointX, globalEndPointY, globalEndPointZ) VALUES "
        "($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17, $18, $19) RETURNING mcparticle_nr;");

    connection->prepare("add_depositedcharge_with_ref",
                        "INSERT INTO DepositedCharge (run_nr, event_nr, mcparticle_nr, detector, carriertype, charge, "
                        "localx, localy, localz, globalx, globaly, globalz) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, "
                        "$10, $11, $12) RETURNING depositedcharge_nr;");
    connection->prepare(
        "add_depositedcharge",
        "INSERT INTO DepositedCharge (run_nr, event_nr, detector, carriertype, charge, localx, localy, localz, globalx, "
        "globaly, globalz) VALUES ( $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11) RETURNING depositedcharge_nr;");

    connection->prepare("add_propagatedcharge_with_ref",
                        "INSERT INTO PropagatedCharge (run_nr, event_nr, depositedcharge_nr, detector, carriertype, charge, "
                        "localx, localy, localz, globalx, globaly, globalz) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, "
                        "$10, $11, $12) RETURNING propagatedcharge_nr;");
    connection->prepare(
        "add_propagatedcharge",
        "INSERT INTO PropagatedCharge (run_nr, event_nr, detector, carriertype, charge, localx, localy, localz, globalx, "
        "globaly, globalz) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11) RETURNING propagatedcharge_nr;");

    connection->prepare(
        "add_pixelcharge_with_ref",
        "INSERT INTO PixelCharge (run_nr, event_nr, propagatedcharge_nr, detector, charge, x, y, localx, localy, globalx, "
        "globaly) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11) RETURNING pixelCharge_nr");
    connection->prepare("add_pixelcharge",
                        "INSERT INTO PixelCharge (run_nr, event_nr, detector, charge, x, y, localx, localy, globalx, "
                        "globaly) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10) RETURNING pixelCharge_nr");

    connection->prepare("add_pixelhit_with_refs",
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
    W_ = std::make_shared<pqxx::nontransaction>(*conn_);

    // inserting run entry in the database
    pqxx::result runR = W_->exec("INSERT INTO Run (run_id) VALUES ('" + run_id_ + "') RETURNING run_nr;");
    run_nr_ = atoi(runR[0][0].c_str());

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

        // Get the detector name
        std::string detector_name;
        if(message->getDetector() != nullptr) {
            detector_name = message->getDetector()->getName();
        }

        // Read the object
        auto object_array = message->getObjectArray();
        if(!object_array.empty()) {
            const Object& first_object = object_array[0];
            auto* cls = TClass::GetClass(typeid(first_object));

            // Remove the allpix prefix
            std::string class_name = cls->GetName();
            std::string apx_namespace = "allpix::";
            size_t ap_idx = class_name.find(apx_namespace);
            if(ap_idx != std::string::npos) {
                class_name.replace(ap_idx, apx_namespace.size(), "");
            }

            // Check if this message should be kept
            if((!include_.empty() && include_.find(class_name) == include_.end()) ||
               (!exclude_.empty() && exclude_.find(class_name) != exclude_.end())) {
                LOG(TRACE) << "Database writer ignored message with object " << allpix::demangle(typeid(*inst).name())
                           << " because it has been excluded or not explicitly included";
                return false;
            }

            return true;
        }

    } catch(MessageWithoutObjectException& e) {
        const BaseMessage* inst = message.get();
        LOG(WARNING) << "Database writer cannot process message of type" << allpix::demangle(typeid(*inst).name())
                     << " with name " << message_name;
    }

    return false;
}

void DatabaseWriterModule::run(Event* event) {
    auto messages = messenger_->fetchFilteredMessages(this, event);

    // TO BE NOTED
    // the correct relations of objects in the database are guaranteed by the fact that sequence of dispatched messages
    // within one event always follows this order: MCTrack -> MCParticle -> DepositedCharge -> PropagatedCharge ->
    // PixelCharge -> PixelHit

    // initializing database referenced parameters to negative
    // if negative values are retained (i.e. the corresponding object is excluded), no reference is created when inserting a
    // new entry in the table
    int mctrack_nr = -1;
    int mcparticle_nr = -1;
    int depositedcharge_nr = -1;
    int propagatedcharge_nr = -1;
    int pixelcharge_nr = -1;

    LOG(TRACE) << "Writing new objects to database";

    std::stringstream insertionLine;

    // Writing entry to event table
    auto insertionResult = W_->exec_prepared("add_event", run_nr_, event->number);
    int event_nr = atoi(insertionResult[0][0].c_str());

    // Looping through messages
    for(auto& pair : messages) {
        auto& message = pair.first;

        // Storing detector's name
        std::string detectorName = "";
        if(message->getDetector() != nullptr) {
            detectorName = message->getDetector()->getName();
        } else {
            detectorName = "global";
        }
        for(auto& object : message->getObjectArray()) {
            // Retrieving object type
            Object& current_object = object;
            auto* cls = TClass::GetClass(typeid(current_object));
            std::string class_name = cls->GetName();
            std::string apx_namespace = "allpix::";
            size_t ap_idx = class_name.find(apx_namespace);
            if(ap_idx != std::string::npos) {
                class_name.replace(ap_idx, apx_namespace.size(), "");
            }
            // Writing objects to corresponding database tables
            if(class_name == "PixelHit") {
                LOG(TRACE) << "inserting PixelHit" << std::endl;
                auto hit = static_cast<PixelHit&>(current_object);
                insertionResult = W_->exec_prepared("add_pixelhit_with_refs",
                                                    run_nr_,
                                                    event_nr,
                                                    nullptr, //mcparticle_nr,
                                                    nullptr, //pixelcharge_nr,
                                                    detectorName,
                                                    hit.getIndex().X(),
                                                    hit.getIndex().Y(),
                                                    hit.getSignal(),
                                                    (timing_global_ ? hit.getGlobalTime() : hit.getLocalTime()));
            } else if(class_name == "PixelCharge") {
                LOG(TRACE) << "inserting PixelCharge" << std::endl;
                auto charge = static_cast<PixelCharge&>(current_object);
                if(propagatedcharge_nr >= 0) {
                    insertionResult = W_->exec_prepared("add_pixelcharge_with_ref",
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
                } else {
                    insertionResult = W_->exec_prepared("add_pixelcharge",
                                                        run_nr_,
                                                        event_nr,
                                                        detectorName,
                                                        charge.getCharge(),
                                                        charge.getIndex().X(),
                                                        charge.getIndex().Y(),
                                                        charge.getPixel().getLocalCenter().X(),
                                                        charge.getPixel().getLocalCenter().Y(),
                                                        charge.getPixel().getGlobalCenter().X(),
                                                        charge.getPixel().getGlobalCenter().Y());
                }
                pixelcharge_nr = atoi(insertionResult[0][0].c_str());
            } else if(class_name == "PropagatedCharge") { // not recommended, this will slow down the simulation considerably
                LOG(TRACE) << "inserting PropagatedCharge" << std::endl;
                PropagatedCharge charge = static_cast<PropagatedCharge&>(current_object);
                if(depositedcharge_nr >= 0) {
                    insertionResult = W_->exec_prepared("add_propagatedcharge_with_ref",
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
                } else {
                    insertionResult = W_->exec_prepared("add_propagatedcharge",
                                                        run_nr_,
                                                        event_nr,
                                                        detectorName,
                                                        static_cast<int>(charge.getType()),
                                                        charge.getCharge(),
                                                        charge.getLocalPosition().X(),
                                                        charge.getLocalPosition().Y(),
                                                        charge.getLocalPosition().Z(),
                                                        charge.getGlobalPosition().X(),
                                                        charge.getGlobalPosition().Y(),
                                                        charge.getGlobalPosition().Z());
                }
                propagatedcharge_nr = atoi(insertionResult[0][0].c_str());
            } else if(class_name == "MCTrack") {
                LOG(TRACE) << "inserting MCTrack" << std::endl;
                auto track = static_cast<MCTrack&>(current_object);
                insertionResult = W_->exec_prepared("add_mctrack",
                                                    run_nr_,
                                                    event_nr,
                                                    detectorName,
                                                    reinterpret_cast<uintptr_t>(&current_object),
                                                    reinterpret_cast<uintptr_t>(track.getParent()),
                                                    track.getParticleID(),
                                                    track.getCreationProcessName(),
                                                    track.getOriginatingVolumeName(),
                                                    track.getStartPoint().X(),
                                                    track.getStartPoint().Y(),
                                                    track.getStartPoint().Z(),
                                                    track.getEndPoint().X(),
                                                    track.getEndPoint().Y(),
                                                    track.getEndPoint().Z(),
                                                    track.getKineticEnergyInitial(),
                                                    track.getKineticEnergyFinal());
                mctrack_nr = atoi(insertionResult[0][0].c_str());
            } else if(class_name == "DepositedCharge") {
                LOG(TRACE) << "inserting DepositedCharge" << std::endl;
                auto charge = static_cast<DepositedCharge&>(current_object);
                if(mcparticle_nr >= 0) {
                    insertionResult = W_->exec_prepared("add_depositedcharge_with_ref",
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
                } else {
                    insertionResult = W_->exec_prepared("add_depositedcharge",
                                                        run_nr_,
                                                        event_nr,
                                                        detectorName,
                                                        static_cast<int>(charge.getType()),
                                                        charge.getCharge(),
                                                        charge.getLocalPosition().X(),
                                                        charge.getLocalPosition().Y(),
                                                        charge.getLocalPosition().Z(),
                                                        charge.getGlobalPosition().X(),
                                                        charge.getGlobalPosition().Y(),
                                                        charge.getGlobalPosition().Z());
                }
                depositedcharge_nr = atoi(insertionResult[0][0].c_str());
            } else if(class_name == "MCParticle") {
                LOG(TRACE) << "inserting MCParticle" << std::endl;
                auto particle = static_cast<MCParticle&>(current_object);
                if(mctrack_nr >= 0) {
                    insertionResult = W_->exec_prepared("add_mcparticle_with_ref",
                                                        run_nr_,
                                                        event_nr,
                                                        mctrack_nr,
                                                        detectorName,
                                                        reinterpret_cast<uintptr_t>(&current_object),
                                                        reinterpret_cast<uintptr_t>(particle.getParent()),
                                                        reinterpret_cast<uintptr_t>(particle.getTrack()),
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
                } else {
                    insertionResult = W_->exec_prepared("add_mcparticle",
                                                        run_nr_,
                                                        event_nr,
                                                        detectorName,
                                                        reinterpret_cast<uintptr_t>(&current_object),
                                                        reinterpret_cast<uintptr_t>(particle.getParent()),
                                                        reinterpret_cast<uintptr_t>(particle.getTrack()),
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
                }
                mcparticle_nr = atoi(insertionResult[0][0].c_str());
            } else {
                LOG(WARNING) << "Following object type is not yet accounted for in database output: " << class_name
                             << std::endl;
            }
            write_cnt_++;
        }
        msg_cnt_++;
    }
}

void DatabaseWriterModule::finalizeThread() {
    // disconnecting from database
    conn_->disconnect();
}

void DatabaseWriterModule::finalize() {

    // Print statistics
    LOG(STATUS) << "Wrote " << write_cnt_ << " objects from " << msg_cnt_ << " messages to database" << std::endl;
}
