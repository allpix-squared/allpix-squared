#include "ROOTObjectWriterModule.hpp"

#include <string>
#include <utility>

#include <TBranchElement.h>
#include <TClass.h>

#include "core/utils/log.h"
#include "core/utils/type.h"

#include "objects/Object.hpp"
#include "objects/objects.h"

using namespace allpix;

ROOTObjectWriterModule::ROOTObjectWriterModule(Configuration config, Messenger* messenger, GeometryManager*)
    : Module(config), config_(std::move(config)) {
    // Bind to all messages
    messenger->registerListener(this, &ROOTObjectWriterModule::receive);
}
ROOTObjectWriterModule::~ROOTObjectWriterModule() {
    // Delete all pointers
    // NOTE: cannot be smart pointers due to internal ROOT logic
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
    output_file_->Write();

    int branch_count = 0;
    for(auto& tree : trees_) {
        branch_count += tree.second->GetListOfBranches()->GetEntries();
    }

    // Print statistics
    LOG(INFO) << "Written " << write_cnt_ << " objects to " << branch_count << " branches";
}
