/**
 * @file
 * @brief Implementation of ROOT data file writer module
 *
 * @copyright Copyright (c) 2018-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "TextWriterModule.hpp"

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

TextWriterModule::TextWriterModule(Configuration& config, Messenger* messenger, GeometryManager*)
    : SequentialModule(config), messenger_(messenger) {
    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();

    // Bind to all messages with filter
    messenger_->registerFilter(this, &TextWriterModule::filter);
}

void TextWriterModule::initialize() {
    // Create output file
    output_file_name_ = createOutputFile(config_.get<std::string>("file_name", "data"), "txt", true);
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

bool TextWriterModule::filter(const std::shared_ptr<BaseMessage>& message, const std::string& message_name) const { // NOLINT
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
            std::string class_name = allpix::demangle(typeid(first_object).name());

            // Check if this message should be kept
            if((!include_.empty() && include_.find(class_name) == include_.end()) ||
               (!exclude_.empty() && exclude_.find(class_name) != exclude_.end())) {
                LOG(TRACE) << "Text writer ignored message with object " << allpix::demangle(typeid(*inst).name())
                           << " because it has been excluded or not explicitly included";
                return false;
            }

            return true;
        }

    } catch(MessageWithoutObjectException& e) {
        const BaseMessage* inst = message.get();
        LOG(WARNING) << "Text writer cannot process message of type" << allpix::demangle(typeid(*inst).name())
                     << " with name " << message_name;
    }

    return false;
}

void TextWriterModule::run(Event* event) {
    auto messages = messenger_->fetchFilteredMessages(this, event);
    LOG(TRACE) << "Writing new objects to text file";

    // Print the current event:
    *output_file_ << "=== " << event->number << " ===" << std::endl;

    for(auto& pair : messages) {
        auto& message = pair.first;

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
}

void TextWriterModule::finalize() {
    // Finish writing to output file
    *output_file_ << "# " << write_cnt_ << " objects from " << msg_cnt_ << " messages" << std::endl;

    // Print statistics
    LOG(STATUS) << "Wrote " << write_cnt_ << " objects from " << msg_cnt_ << " messages to file:" << std::endl
                << output_file_name_;
}
