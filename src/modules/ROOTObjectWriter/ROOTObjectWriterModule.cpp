#include "ROOTObjectWriterModule.hpp"

#include <string>
#include <utility>

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
    output_file_ = std::make_unique<TFile>(getOutputPath("data.root").c_str(), "RECREATE");
    output_file_->cd();

    tree_ = std::make_unique<TTree>("tree", "");
}

void ROOTObjectWriterModule::receive(std::shared_ptr<BaseMessage> message, std::string message_name) { // NOLINT
    try {
        const BaseMessage* inst = message.get();
        LOG(TRACE) << "Received " << allpix::demangle(typeid(*inst).name()) << " named " << message_name;

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
                tree_->Bronch((detector_name + "_" + cls->GetName() + "_" + message_name).c_str(),
                              (std::string("std::vector<") + cls->GetName() + "*>").c_str(),
                              addr);
            }

            // Fill the branch vector
            for(Object& object : object_array) {
                ++write_cnt_;
                write_list_[index_tuple]->push_back(&object);
            }
        }

    } catch(MessageWithoutObjectException& e) {
        const BaseMessage* inst = message.get();
        LOG(WARNING) << "Cannot process message of type" << allpix::demangle(typeid(*inst).name()) << " with name "
                     << message_name;
    }
}

void ROOTObjectWriterModule::run(unsigned int) {
    LOG(TRACE) << "Writing new objects to tree";
    output_file_->cd();

    // Fill the tree with the current received messages
    tree_->Fill();

    // Clear the current message list
    for(auto& index_data : write_list_) {
        index_data.second->clear();
    }
    // Clear the messages we have to keep because they contain the internal pointers
    keep_messages_.clear();
}

void ROOTObjectWriterModule::finalize() {
    LOG(TRACE) << "Writing objects to file";

    // Print statistics
    LOG(INFO) << "Written " << write_cnt_ << " objects to " << tree_->GetListOfBranches()->GetEntries() << " branches";

    // Write the tree to the output file
    output_file_->Write();
}
