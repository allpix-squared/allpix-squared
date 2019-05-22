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

ROOTObjectWriterModule::ROOTObjectWriterModule(Configuration& config, Messenger* messenger, GeometryManager* geo_mgr)
    : WriterModule(config), geo_mgr_(geo_mgr) {
    // Bind to all messages with filter
    messenger->registerFilter(this, &ROOTObjectWriterModule::filter);
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

void ROOTObjectWriterModule::init(std::mt19937_64&) {
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

bool ROOTObjectWriterModule::filter(const std::shared_ptr<BaseMessage>& message,
                                    const std::string& message_name) const { // NOLINT
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
        if(object_array.empty()) {
            return false;
        }
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
        if((!include_.empty() && include_.find(class_name) == include_.cend()) ||
           (!exclude_.empty() && exclude_.find(class_name) != exclude_.cend())) {
            LOG(TRACE) << "ROOT object writer ignored message with object " << allpix::demangle(typeid(*inst).name())
                       << " because it has been excluded or not explicitly included";
            return false;
        }
    } catch(MessageWithoutObjectException& e) {
        const BaseMessage* inst = message.get();
        LOG(WARNING) << "ROOT object writer cannot process message of type" << allpix::demangle(typeid(*inst).name())
                     << " with name " << message_name;
        return false;
    }

    return true;
}

void ROOTObjectWriterModule::pre_run(Event* event) const {
    auto messages = event->fetchFilteredMessages();

    for(auto& pair : messages) {
        auto& message = pair.first;
        auto& message_name = pair.second;

        // Get the detector name
        std::string detector_name;
        if(message->getDetector() != nullptr) {
            detector_name = message->getDetector()->getName();
        }

        // Read the object
        auto object_array = message->getObjectArray();
        // object_array emptiness is checked in the filter
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

            // Add vector of objects to write to the write list
            write_list_[index_tuple] = new std::vector<Object*>();
            auto addr = &write_list_[index_tuple];

            if(trees_.find(class_name) == trees_.end()) {
                // Create new tree
                output_file_->cd();
                trees_.emplace(class_name,
                               std::make_unique<TTree>(class_name.c_str(), (std::string("Tree of ") + class_name).c_str()));
            }

            std::string branch_name = detector_name.empty() ? "global" : detector_name;
            if(!message_name.empty()) {
                branch_name += "_";
                branch_name += message_name;
            }

            trees_[class_name]->Bronch(
                branch_name.c_str(), (std::string("std::vector<") + cls->GetName() + "*>").c_str(), addr);
        }

        // Fill the branch vector
        for(Object& object : object_array) {
            std::lock_guard<std::mutex> lock{stats_mutex_};
            ++write_cnt_;
            write_list_[index_tuple]->push_back(&object);
        }
    }
}

void ROOTObjectWriterModule::run(Event* event) const {
    // Generate trees and index data
    pre_run(event);

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

    // Get the config manager
    ConfigManager* conf_manager = getConfigManager();

    // Save the main configuration to the output file
    auto global_dir = config_dir->mkdir("Allpix");
    LOG(TRACE) << "Writing global configuration";

    // Loop over all values in the global configuration
    for(auto& key_value : conf_manager->getGlobalConfiguration().getAll()) {
        global_dir->WriteObject(&key_value.second, key_value.first.c_str());
    }

    // Save the instance configuration to the output file
    for(auto& config : conf_manager->getInstanceConfigurations()) {
        // Create a new directory per section, using the unique module name
        auto unique_name = config.getName();
        auto identifier = config.get<std::string>("identifier");
        if(!identifier.empty()) {
            unique_name += ":";
            unique_name += identifier;
        }
        auto section_dir = config_dir->mkdir(unique_name.c_str());
        LOG(TRACE) << "Writing configuration for: " << unique_name;

        // Loop over all values in the section
        for(auto& key_value : config.getAll()) {
            // Skip the identifier
            if(key_value.first == "identifier") {
                continue;
            }
            section_dir->WriteObject(&key_value.second, key_value.first.c_str());
        }
    }

    // Save the detectors to the output file
    auto detectors_dir = output_file_->mkdir("detectors");
    auto models_dir = output_file_->mkdir("models");
    for(auto& detector : geo_mgr_->getDetectors()) {
        detectors_dir->cd();
        LOG(TRACE) << "Writing detector configuration for: " << detector->getName();
        auto detector_dir = detectors_dir->mkdir(detector->getName().c_str());

        auto position = detector->getPosition();
        detector_dir->WriteObject(&position, "position");
        auto orientation = detector->getOrientation();
        detector_dir->WriteObject(&orientation, "orientation");

        // Store the detector model
        // NOTE We save the model for every detector separately since parameter overloading might have changed it
        std::string model_name = detector->getModel()->getType() + "_" + detector->getName();
        detector_dir->WriteObject(&model_name, "type");
        models_dir->cd();
        auto model_dir = models_dir->mkdir(model_name.c_str());

        // Get all sections of the model configuration (maon config plus support layers):
        auto model_configs = detector->getModel()->getConfigurations();
        std::map<std::string, int> count_configs;
        for(auto& model_config : model_configs) {
            auto model_config_dir = model_dir;
            if(!model_config.getName().empty()) {
                model_config_dir = model_dir->mkdir(
                    (model_config.getName() + "_" + std::to_string(count_configs[model_config.getName()])).c_str());
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

// check if the required messages are present in the event
bool ROOTObjectWriterModule::isSatisfied(Event* event) const {
    try {
        auto messages = event->fetchFilteredMessages();
    } catch (std::out_of_range&) {
        return false;
    }

    return true;
}
