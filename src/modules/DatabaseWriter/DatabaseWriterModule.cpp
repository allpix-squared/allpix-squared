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
            //	    std::cout << "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA class_name = " << class_name << std::endl;
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

    // Writing entry to event table
    char eventLine[1000];
    sprintf(eventLine, "INSERT INTO Event (run_nr, eventID) VALUES (currval('run_run_nr_seq'), %d)", event_num);
    result binR = W_->exec(eventLine);

    // Looping through messages
    for(auto& message : keep_messages_) {
        // Save detector's name
        std::string detectorName = "";
        if(message->getDetector() != nullptr) {
            //	std::cout << "--- " << message->getDetector()->getName() << " ---" << std::endl;
            detectorName = message->getDetector()->getName();
        } else {
            //	std::cout << "--- <global> ---" << std::endl;
            detectorName = "global";
        }
        for(auto& object : message->getObjectArray()) {
            // Print the object's ASCII representation:
            //	*output_file_ << object << std::endl;
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
