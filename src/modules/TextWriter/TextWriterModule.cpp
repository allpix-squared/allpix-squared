/**
 * @file
 * @brief Implementation of ROOT data file writer module
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "TextWriterModule.hpp"

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

TextWriterModule::TextWriterModule(Configuration& config, Messenger* messenger, GeometryManager*) : Module(config) {
    // Bind to all messages
    messenger->registerListener(this, &TextWriterModule::receive);
}
/**
 * @note Objects cannot be stored in smart pointers due to internal ROOT logic
 */
TextWriterModule::~TextWriterModule() {
    // Delete all object pointers
    for(auto& index_data : write_list_) {
        delete index_data.second;
    }
}

void TextWriterModule::init() {
    // Create output file
    output_file_name_ =
        createOutputFile(allpix::add_file_extension(config_.get<std::string>("file_name", "data"), "txt"), true);
    output_file_ = std::make_unique<std::ofstream>(output_file_name_);

    *output_file_ << "# Allpix Squared ASCII data - https://cern.ch/allpix-squared" << std::endl << std::endl;

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

void TextWriterModule::receive(std::shared_ptr<BaseMessage> message, std::string message_name) { // NOLINT
    try {
        const BaseMessage* inst = message.get();
        std::string name_str = " without a name";
        if(!message_name.empty()) {
            name_str = " named " + message_name;
        }
        LOG(TRACE) << "Text writer received " << allpix::demangle(typeid(*inst).name()) << name_str;

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
                LOG(TRACE) << "Text writer ignored message with object " << allpix::demangle(typeid(*inst).name())
                           << " because it has been excluded or not explicitly included";
                return;
            }

            // Store message for later reference
            keep_messages_.push_back(message);
        }

    } catch(MessageWithoutObjectException& e) {
        const BaseMessage* inst = message.get();
        LOG(WARNING) << "Text writer cannot process message of type" << allpix::demangle(typeid(*inst).name())
                     << " with name " << message_name;
    }
}

void TextWriterModule::run(unsigned int event_num) {
    LOG(TRACE) << "Writing new objects to text file";

    // Print the current event:
    *output_file_ << "=== " << event_num << " ===" << std::endl;

    for(auto& message : keep_messages_) {
        // Print the current detector:
        if(message->getDetector() != nullptr) {
            *output_file_ << "--- " << message->getDetector()->getName() << " ---" << std::endl;
        } else {
            *output_file_ << "--- <global> ---" << std::endl;
        }
        for(auto& object : message->getObjectArray()) {
            // Print the object's ASCII representation:
            *output_file_ << object << std::endl;
            write_cnt_++;
        }
        msg_cnt_++;
    }

    // Clear the messages we have to keep because they contain the internal pointers
    keep_messages_.clear();
}

void TextWriterModule::finalize() {
    // Finish writing to output file
    *output_file_ << "# " << write_cnt_ << " objects from " << msg_cnt_ << " messages" << std::endl;

    // Print statistics
    LOG(STATUS) << "Wrote " << write_cnt_ << " objects from " << msg_cnt_ << " messages to file:" << std::endl
                << output_file_name_;
}
