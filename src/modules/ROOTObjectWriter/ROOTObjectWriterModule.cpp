/**
 * @file
 * @brief Implementation of ROOT data file writer module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "ROOTObjectWriterModule.hpp"

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

ROOTObjectWriterModule::ROOTObjectWriterModule(Configuration config, Messenger* messenger, GeometryManager* geo_mgr)
    : Module(std::move(config)), geo_mgr_(geo_mgr) {
    // Bind to all messages
    messenger->registerListener(this, &ROOTObjectWriterModule::receive);
}
/**
 * @note Objects cannot be stored in smart pointers due to internal ROOT logic
 */
ROOTObjectWriterModule::~ROOTObjectWriterModule() {
    // Delete all object pointers
    for(auto& index_data : write_list_) {
        delete index_data.second;
    }
}

void ROOTObjectWriterModule::init() {
    // Create output file
    output_file_name_ =
        createOutputFile(allpix::add_file_extension(config_.get<std::string>("file_name", "data"), "root"), true);
    output_file_ = std::make_unique<TFile>(output_file_name_.c_str(), "RECREATE");
    output_file_->cd();

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

void ROOTObjectWriterModule::receive(std::shared_ptr<BaseMessage> message, std::string message_name) { // NOLINT
    try {
        const BaseMessage* inst = message.get();
        std::string name_str = " without a name";
        if(!message_name.empty()) {
            name_str = " named " + message_name;
        }
        LOG(TRACE) << "ROOT object writer received " << allpix::demangle(typeid(*inst).name()) << name_str;

        // Get the detector name
        std::string detector_name;
        if(message->getDetector() != nullptr) {
            detector_name = message->getDetector()->getName();
        }

        // Read the object
        auto object_array = message->getObjectArray();
        if(!object_array.empty()) {
            keep_messages_.push_back(message);

            const Object& first_object = object_array[0];
            std::type_index type_idx = typeid(first_object);

            // Create a new branch of the correct type if this message was not received before
            auto index_tuple = std::make_tuple(type_idx, detector_name, message_name);
            if(write_list_.find(index_tuple) == write_list_.end()) {

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
                    LOG(TRACE) << "ROOT object writer ignored message with object " << allpix::demangle(typeid(*inst).name())
                               << " because it has been excluded or not explicitly included";
                    return;
                }

                // Add vector of objects to write to the write list
                write_list_[index_tuple] = new std::vector<Object*>();
                auto addr = &write_list_[index_tuple];

                if(trees_.find(class_name) == trees_.end()) {
                    // Create new tree
                    output_file_->cd();
                    trees_.emplace(
                        class_name,
                        std::make_unique<TTree>(class_name.c_str(), (std::string("Tree of ") + class_name).c_str()));
                }

                std::string branch_name = detector_name;
                if(!message_name.empty()) {
                    branch_name += "_";
                    branch_name += message_name;
                }

                trees_[class_name]->Bronch(
                    branch_name.c_str(), (std::string("std::vector<") + cls->GetName() + "*>").c_str(), addr);
            }

            // Fill the branch vector
            for(Object& object : object_array) {
                ++write_cnt_;
                write_list_[index_tuple]->push_back(&object);
            }
        }

    } catch(MessageWithoutObjectException& e) {
        const BaseMessage* inst = message.get();
        LOG(WARNING) << "ROOT object writer cannot process message of type" << allpix::demangle(typeid(*inst).name())
                     << " with name " << message_name;
    }
}

void ROOTObjectWriterModule::run(unsigned int) {
    LOG(TRACE) << "Writing new objects to tree";
    output_file_->cd();

    // Fill the tree with the current received messages
    for(auto& tree : trees_) {
        tree.second->Fill();
    }

    // Clear the current message list
    for(auto& index_data : write_list_) {
        index_data.second->clear();
    }
    // Clear the messages we have to keep because they contain the internal pointers
    keep_messages_.clear();
}

void ROOTObjectWriterModule::finalize() {
    LOG(TRACE) << "Writing objects to file";
    output_file_->cd();

    int branch_count = 0;
    for(auto& tree : trees_) {
        // Update statistics
        branch_count += tree.second->GetListOfBranches()->GetEntries();
    }

    // Create main config directory
    TDirectory* config_dir = output_file_->mkdir("config");
    config_dir->cd();

    // Save the main configuration to the output file
    for(auto& config : this->get_final_configuration()) {
        // Create a new directory per section, using the unique module identifiers as names
        auto section_dir = config_dir->mkdir(config.getName().c_str());
        LOG(TRACE) << "Writing configuration for: " << config.getName();

        // Loop over all values in the section
        for(auto& key_value : config.getAll()) {
            section_dir->WriteObject(&key_value.second, key_value.first.c_str());
        }
    }

    // Save the detectors to the output file
    // FIXME Possibly the format to save the geometry should be more flexible
    auto detectors_dir = output_file_->mkdir("detectors");
    detectors_dir->cd();
    for(auto& detector : geo_mgr_->getDetectors()) {
        auto detector_dir = detectors_dir->mkdir(detector->getName().c_str());

        auto position = detector->getPosition();
        detector_dir->WriteObject(&position, "position");
        auto orientation = detector->getOrientation();
        detector_dir->WriteObject(&orientation, "orientation");

        auto model_dir = detector_dir->mkdir("model");
        // FIXME This saves the model every time again also for models that appear multiple times
        auto model_configs = detector->getModel()->getConfigurations();
        std::map<std::string, int> count_configs;
        for(auto& model_config : model_configs) {
            auto model_config_dir = model_dir;
            if(!model_config.getName().empty()) {
                model_config_dir = model_dir->mkdir(
                    (model_config.getName() + "-" + std::to_string(count_configs[model_config.getName()])).c_str());
                count_configs[model_config.getName()]++;
            }

            for(auto& key_value : model_config.getAll()) {
                model_config_dir->WriteObject(&key_value.second, key_value.first.c_str());
            }
        }
    }

    // Finish writing to output file
    output_file_->Write();

    // Print statistics
    LOG(STATUS) << "Wrote " << write_cnt_ << " objects to " << branch_count << " branches in file:" << std::endl
                << output_file_name_;
}
