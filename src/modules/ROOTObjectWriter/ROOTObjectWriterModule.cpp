/**
 * @file
 * @brief Implementation of ROOT data file writer module
 * @copyright MIT License
 */

#include "ROOTObjectWriterModule.hpp"

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

ROOTObjectWriterModule::ROOTObjectWriterModule(Configuration config, Messenger* messenger, GeometryManager* geo_mgr)
    : Module(config), config_(std::move(config)), geo_mgr_(geo_mgr) {
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
    std::string file_name = getOutputPath(config_.get<std::string>("file_name", "data") + ".root", true);
    output_file_ = std::make_unique<TFile>(file_name.c_str(), "RECREATE");
    output_file_->cd();
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
                write_list_[index_tuple] = new std::vector<Object*>();

                auto* cls = TClass::GetClass(typeid(first_object));
                auto addr = &write_list_[index_tuple];

                // Remove the allpix prefix
                std::string class_name = cls->GetName();
                std::string apx_namespace = "allpix::";
                size_t ap_idx = class_name.find(apx_namespace);
                if(ap_idx != std::string::npos) {
                    class_name.replace(ap_idx, apx_namespace.size(), "");
                }

                if(trees_.find(class_name) == trees_.end()) {
                    // Create new tree
                    output_file_->cd();
                    auto insert_result = trees_.emplace(
                        class_name,
                        std::make_unique<TTree>(class_name.c_str(), (std::string("Tree of ") + class_name).c_str()));

                    // Enable saving references
                    insert_result.first->second->BranchRef();
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

    // Save the main configuration to the output file if possible
    // FIXME This should be improved to write the information in a more flexible way
    std::string path = config_.getFilePath();
    if(!path.empty()) {
        // Create main config directory
        TDirectory* config_dir = output_file_->mkdir("config");
        config_dir->cd();

        // Read the configuration
        std::fstream file(path);
        ConfigReader full_config(file);

        // Loop over all configurations
        std::map<std::string, int> count_configs;
        for(auto& config : full_config.getConfigurations()) {
            // Create a new directory per section (adding a number to make every folder unique)
            // FIXME Writing with the number is not a very good approach
            TDirectory* section_dir =
                config_dir->mkdir((config.getName() + "-" + std::to_string(count_configs[config.getName()])).c_str());
            count_configs[config.getName()]++;

            // Loop over all values in the section
            for(auto& key_value : config.getAll()) {
                section_dir->WriteObject(&key_value.second, key_value.first.c_str());
            }
        }
    } else {
        LOG(ERROR) << "Cannot save main configuration, because the ROOTObjectWriter is not loaded directly from the file";
    }

    // Save the detectors to the output file
    // FIXME Possibly the format to save the geometry should be more flexible
    TDirectory* detectors_dir = output_file_->mkdir("detectors");
    detectors_dir->cd();
    for(auto& detector : geo_mgr_->getDetectors()) {
        TDirectory* detector_dir = detectors_dir->mkdir(detector->getName().c_str());

        auto position = detector->getPosition();
        detector_dir->WriteObject(&position, "position");
        auto orientation = detector->getOrientation();
        detector_dir->WriteObject(&orientation, "orientation");

        TDirectory* model_dir = detector_dir->mkdir("model");
        // FIXME This saves the model every time again also for models that appear multiple times
        for(auto& key_value : detector->getModel()->getConfiguration().getAll()) {
            model_dir->WriteObject(&key_value.second, key_value.first.c_str());
        }
    }

    // Finish writing to output file
    output_file_->Write();

    // Print statistics
    LOG(INFO) << "Written " << write_cnt_ << " objects to " << branch_count << " branches";
}
