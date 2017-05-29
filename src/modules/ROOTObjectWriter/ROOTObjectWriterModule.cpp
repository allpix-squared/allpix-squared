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
    for(auto& detector_data : write_list_) {
        for(auto& type_data : detector_data.second) {
            delete type_data.second;
        }
    }
}

void ROOTObjectWriterModule::init() {
    // Create output file
    output_file_ = std::make_unique<TFile>(getOutputPath("data.root").c_str(), "RECREATE");
    output_file_->cd();

    tree_ = std::make_unique<TTree>("tree", "");
}

void ROOTObjectWriterModule::receive(std::shared_ptr<BaseMessage> message, std::string message_name) {
    try {
        const BaseMessage* inst = message.get();
        LOG(TRACE) << "Received " << allpix::demangle(typeid(*inst).name()) << " named " << message_name;

        // Get the detector name
        std::string detector_name = "";
        if(message->getDetector() != nullptr) {
            detector_name = message->getDetector()->getName();
        }

        // Read the object
        auto object_array = message->getObjectArray();
        if(!object_array.empty()) {
            keep_messages_.push_back(message);

            const Object& object = object_array[0];
            std::type_index type_idx = typeid(object);

            // Create a new branch of the correct type if this message was not received before
            if(write_list_[detector_name].find(type_idx) == write_list_[detector_name].end()) {
                write_list_[detector_name][type_idx] = new std::vector<Object*>();

                auto* cls = TClass::GetClass(typeid(object));
                auto addr = &write_list_[detector_name][type_idx];
                tree_->Bronch((detector_name + "_" + cls->GetName() + "_" + message_name).c_str(),
                              (std::string("std::vector<") + cls->GetName() + "*>").c_str(),
                              addr);
            }

            // Fill the branch vector
            for(size_t i = 0; i < object_array.size(); ++i) {
                Object& object_conv = object_array[i];
                write_list_[detector_name][type_idx]->push_back(&object_conv);
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
    for(auto& detector_data : write_list_) {
        for(auto& type_data : detector_data.second) {
            type_data.second->clear();
        }
    }
    // Clear the messages we have to keep because they contain the internal pointers
    keep_messages_.clear();
}

void ROOTObjectWriterModule::finalize() {
    // ... implement ... (typically you want to fetch some configuration here and in the end possibly output a message)
    LOG(TRACE) << "Writing objects to file";

    // Write the tree to the output file
    output_file_->Write();
}
