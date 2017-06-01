#include "ROOTObjectReaderModule.hpp"

#include <climits>
#include <string>
#include <utility>

#include <TBranch.h>
#include <TBranchElement.h>
#include <TObjArray.h>
#include <TVirtualCollectionProxy.h>

#include "core/messenger/Messenger.hpp"
#include "core/utils/log.h"
#include "core/utils/type.h"

#include "objects/Object.hpp"
#include "objects/objects.h"

#include "core/utils/type.h"

using namespace allpix;

ROOTObjectReaderModule::ROOTObjectReaderModule(Configuration config, Messenger* messenger, GeometryManager* geo_mgr)
    : Module(config), config_(std::move(config)), messenger_(messenger), geo_mgr_(geo_mgr) {}
ROOTObjectReaderModule::~ROOTObjectReaderModule() {
    for(auto message_inf : message_info_array_) {
        delete message_inf.objects;
    }
}

template <typename T> static void add_creator(ROOTObjectReaderModule::MessageCreatorMap& map) {
    map[typeid(T)] = [&](std::vector<Object*> objects, std::shared_ptr<Detector> detector = nullptr) {
        std::vector<T> data;
        for(auto& object : objects) {
            data.emplace_back(std::move(*dynamic_cast<T*>(object)));
        }

        if(detector == nullptr) {
            return std::make_shared<Message<T>>(data);
        }
        return std::make_shared<Message<T>>(data, detector);
    };
}

template <template <typename...> class T, typename... Args>
static void gen_creator_map(ROOTObjectReaderModule::MessageCreatorMap& map, type_tag<T<Args...>>) {
    std::initializer_list<int> value{(add_creator<Args>(map), 0)...};
    (void)value;
}

template <typename T> static ROOTObjectReaderModule::MessageCreatorMap gen_creator_map() {
    ROOTObjectReaderModule::MessageCreatorMap ret_map;
    gen_creator_map(ret_map, type_tag<T>());
    return ret_map;
}

void ROOTObjectReaderModule::init() {
    // Initialize the call map
    message_creator_map_ = gen_creator_map<allpix::OBJECTS>();

    // Open the input file
    input_file_ = std::make_unique<TFile>(config_.getPath("file", true).c_str());

    // Open the tree
    tree_ = dynamic_cast<TTree*>(input_file_->Get("tree"));
    if(tree_ == nullptr) {
        throw ModuleError("Given file does not contain a tree");
    }

    // Loop over the list of branches and create the set of receiver objects
    TObjArray* branches = tree_->GetListOfBranches();
    for(int i = 0; i < branches->GetEntries(); i++) {
        auto* branch = dynamic_cast<TBranch*>(branches->At(i));

        // Add a new vector of objects and bind it to the branch
        message_info message_inf;
        message_inf.objects = new std::vector<Object*>;
        message_info_array_.emplace_back(message_inf);
        branch->SetAddress(&(message_info_array_.back().objects));

        // Fill the rest of the message information
        // FIXME: we want to index this in a different way
        std::string branch_name = branch->GetName();
        auto split = allpix::split<std::string>(branch_name, "_");

        size_t expected_size = 3;
        size_t det_idx = 0;
        size_t type_idx = 1;
        size_t name_idx = 2;
        if(branch_name.front() == '_') {
            --expected_size;
            det_idx = INT_MAX;
            --type_idx;
            --name_idx;
        }
        if(branch_name.back() == '_') {
            --expected_size;
            name_idx = INT_MAX;
        }

        auto split_type = allpix::split<std::string>(branch->GetClassName(), "<>");
        if(expected_size != split.size() || split_type.size() != 2 || split[type_idx] + "*" != split_type[1]) {
            throw ModuleError("Tree is malformed and cannot be used for creating messages");
        }

        std::string message_name;
        if(name_idx != INT_MAX) {
            message_info_array_.back().name = split[name_idx];
        }
        std::shared_ptr<Detector> detector = nullptr;
        if(det_idx != INT_MAX) {
            message_info_array_.back().detector = geo_mgr_->getDetector(split[det_idx]);
        }
    }
}

void ROOTObjectReaderModule::run(unsigned int event_num) {
    --event_num;
    if(event_num >= tree_->GetEntries()) {
        LOG(WARNING) << "Event tree does not contain data for event " << event_num;
        return;
    }
    tree_->GetEntry(event_num);
    LOG(TRACE) << "Building messages from stored objects";

    // Loop through all branches
    for(auto message_inf : message_info_array_) {
        auto objects = message_inf.objects;

        // Skip empty objects in current event
        if(objects->empty()) {
            continue;
        }

        // Check if a pointer to a dispatcher method exist
        auto first_object = (*objects)[0];
        auto iter = message_creator_map_.find(typeid(*first_object));
        if(iter == message_creator_map_.end()) {
            LOG(INFO) << "Cannot dispatch message with object " << allpix::demangle(typeid(*first_object).name())
                      << " because it not registered for messaging";
            continue;
        }

        // Update statistics
        read_cnt_ += objects->size();

        // Create a message
        std::shared_ptr<BaseMessage> message = iter->second(*objects, message_inf.detector);

        // Dispatch the message
        messenger_->dispatchMessage(this, message, message_inf.name);
    }
}

void ROOTObjectReaderModule::finalize() {
    // Print statistics
    LOG(INFO) << "Read " << read_cnt_ << " objects from " << tree_->GetListOfBranches()->GetEntries() << " branches";

    // Close the file
    input_file_->Close();
}
