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
#include "core/utils/file.h"
#include "core/utils/log.h"
#include "core/utils/type.h"

#include "objects/Object.hpp"
#include "objects/objects.h"

using namespace allpix;

DatabaseWriterModule::DatabaseWriterModule(Configuration& config, Messenger* messenger, GeometryManager*) : Module(config) {
    // Bind to all messages
    messenger->registerListener(this, &DatabaseWriterModule::receive);
}
/**
 * @note Objects cannot be stored in smart pointers due to internal ROOT logic
 */
DatabaseWriterModule::~DatabaseWriterModule() {
    // Delete all object pointers
    for(auto& index_data : write_list_) {
        delete index_data.second;
    }
}

void DatabaseWriterModule::init() {

    // retrieving configuration parameters
    host_ = config_.get<std::string>("host", "");
    port_ = config_.get<std::string>("port", "");
    dbname_ = config_.get<std::string>("dbname", "");
    user_ = config_.get<std::string>("user", "");
    password_ = config_.get<std::string>("password", "");
    runID_ = config_.get<std::string>("runID", "");

    // establishing connection to the database
    conn_ = new connection("host=" + host_ + " port=" + port_ + " dbname=" + dbname_ + " user=" + user_ +
                           " password=" + password_);
    if(!conn_->is_open()) {
        std::cout << __PRETTY_FUNCTION__ << ": ERROR!!! - could not connect to database" << std::endl;
        return;
    }

    W_ = new nontransaction(*conn_);

    // inserting run entry in the database
    result runR = W_->exec("INSERT INTO Run (runID) VALUES ('" + runID_ + "') RETURNING run_nr;");
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

void DatabaseWriterModule::receive(std::shared_ptr<BaseMessage> message, std::string message_name) { // NOLINT
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
                return;
            }

            // Store message for later reference
            keep_messages_.push_back(message);
        }

    } catch(MessageWithoutObjectException& e) {
        const BaseMessage* inst = message.get();
        LOG(WARNING) << "Database writer cannot process message of type" << allpix::demangle(typeid(*inst).name())
                     << " with name " << message_name;
    }
}

void DatabaseWriterModule::run(unsigned int event_num) {
    LOG(TRACE) << "Writing new objects to text file";

    char insertionLine[1000];
    result insertionResult;

    // Writing entry to event table
    sprintf(insertionLine, "INSERT INTO Event (run_nr, eventID) VALUES (currval('run_run_nr_seq'), %d)", event_num);
    insertionResult = W_->exec(insertionLine);

    // Looping through messages
    for(auto& message : keep_messages_) {
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
                PixelHit hit = static_cast<PixelHit&>(current_object);
                sprintf(insertionLine,
                        "INSERT INTO PixelHit (run_nr, event_nr, detector, x, y, signal, hittime) VALUES "
                        "(currval('run_run_nr_seq'), currval('event_event_nr_seq'), '%s', %d, %d, %lf, %lf)",
                        detectorName.c_str(),
                        hit.getIndex().X(),
                        hit.getIndex().Y(),
                        hit.getSignal(),
                        hit.getTime());
                insertionResult = W_->exec(insertionLine);
            } else if(class_name == "PixelCharge") {
                PixelCharge charge = static_cast<PixelCharge&>(current_object);
                sprintf(insertionLine,
                        "INSERT INTO PixelCharge (run_nr, event_nr, detector, charge, x, y, localx, localy, globalx, "
                        "globaly) VALUES (currval('run_run_nr_seq'), currval('event_event_nr_seq'), '%s', %d, %d, %d, %lf, "
                        "%lf, %lf, %lf)",
                        detectorName.c_str(),
                        charge.getCharge(),
                        charge.getIndex().X(),
                        charge.getIndex().Y(),
                        charge.getPixel().getLocalCenter().X(),
                        charge.getPixel().getLocalCenter().Y(),
                        charge.getPixel().getGlobalCenter().X(),
                        charge.getPixel().getGlobalCenter().Y());
                insertionResult = W_->exec(insertionLine);
            } else if(class_name == "PropagatedCharge") { // not recommended, this will slow down the simulation considerably
                PropagatedCharge charge = static_cast<PropagatedCharge&>(current_object);
                sprintf(insertionLine,
                        "INSERT INTO PropagatedCharge (run_nr, event_nr, detector, carriertype, charge, localx, localy, "
                        "localz, globalx, globaly, globalz) VALUES (currval('run_run_nr_seq'), "
                        "currval('event_event_nr_seq'), '%s', %d, %d, %lf, %lf, %lf, %lf, %lf, %lf)",
                        detectorName.c_str(),
                        static_cast<int>(charge.getType()),
                        charge.getCharge(),
                        charge.getLocalPosition().X(),
                        charge.getLocalPosition().Y(),
                        charge.getLocalPosition().Z(),
                        charge.getGlobalPosition().X(),
                        charge.getGlobalPosition().Y(),
                        charge.getGlobalPosition().Z());
                insertionResult = W_->exec(insertionLine);
            } else if(class_name == "MCTrack") {
                MCTrack track = static_cast<MCTrack&>(current_object);
                sprintf(insertionLine,
                        "INSERT INTO MCTrack (run_nr, event_nr, detector, address, parentAddress, particleID, "
                        "productionProcess, productionVolume, initialPositionX, initialPositionY, initialPositionZ, "
                        "finalPositionX, finalPositionY, finalPositionZ, initialKineticEnergy, finalKineticEnergy) VALUES "
                        "(currval('run_run_nr_seq'), currval('event_event_nr_seq'), '%s', %ld, %ld, %d, '%s', '%s', %lf, "
                        "%lf, %lf, %lf, %lf, %lf, %lf, %lf)",
                        detectorName.c_str(),
                        reinterpret_cast<uintptr_t>(&current_object),
                        reinterpret_cast<uintptr_t>(track.getParent()),
                        track.getParticleID(),
                        track.getCreationProcessName().c_str(),
                        track.getOriginatingVolumeName().c_str(),
                        track.getStartPoint().X(),
                        track.getStartPoint().Y(),
                        track.getStartPoint().Z(),
                        track.getEndPoint().X(),
                        track.getEndPoint().Y(),
                        track.getEndPoint().Z(),
                        track.getKineticEnergyInitial(),
                        track.getKineticEnergyFinal());
                insertionResult = W_->exec(insertionLine);
            } else if(class_name == "DepositedCharge") {
                DepositedCharge charge = static_cast<DepositedCharge&>(current_object);
                sprintf(insertionLine,
                        "INSERT INTO DepositedCharge (run_nr, event_nr, detector, carriertype, charge, localx, localy, "
                        "localz, globalx, globaly, globalz) VALUES (currval('run_run_nr_seq'), "
                        "currval('event_event_nr_seq'), '%s', %d, %d, %lf, %lf, %lf, %lf, %lf, %lf)",
                        detectorName.c_str(),
                        static_cast<int>(charge.getType()),
                        charge.getCharge(),
                        charge.getLocalPosition().X(),
                        charge.getLocalPosition().Y(),
                        charge.getLocalPosition().Z(),
                        charge.getGlobalPosition().X(),
                        charge.getGlobalPosition().Y(),
                        charge.getGlobalPosition().Z());
                insertionResult = W_->exec(insertionLine);
            } else if(class_name == "MCParticle") {
                MCParticle particle = static_cast<MCParticle&>(current_object);
                sprintf(
                    insertionLine,
                    "INSERT INTO MCParticle (run_nr, event_nr, detector, address, parentAddress, trackAddress, particleID, "
                    "localStartPointX, localStartPointY, localStartPointZ, localEndPointX, localEndPointY, localEndPointZ, "
                    "globalStartPointX, globalStartPointY, globalStartPointZ, globalEndPointX, globalEndPointY, "
                    "globalEndPointZ) VALUES (currval('run_run_nr_seq'), currval('event_event_nr_seq'), '%s', %ld, %ld, "
                    "%ld, %d, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf)",
                    detectorName.c_str(),
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
                insertionResult = W_->exec(insertionLine);
            } else {
                LOG(WARNING) << "Following object type is not yet accounted for in database output: " << class_name
                             << std::endl;
            }
            write_cnt_++;
        }
        msg_cnt_++;
    }

    // Clear the messages we have to keep because they contain the internal pointers
    keep_messages_.clear();
}

void DatabaseWriterModule::finalize() {

    // disconnecting from database
    delete W_;
    conn_->disconnect();

    // Print statistics
    LOG(STATUS) << "Wrote " << write_cnt_ << " objects from " << msg_cnt_ << " messages to database" << std::endl;
}
