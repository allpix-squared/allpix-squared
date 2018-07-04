/**
 * @file
 * @brief Implementation of messenger
 *
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Messenger.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>

#include "Message.hpp"
#include "core/module/Module.hpp"
#include "core/utils/log.h"
#include "core/utils/type.h"
#include "delegates.h"

using namespace allpix;

Messenger::Messenger() = default;
#ifdef NDEBUG
Messenger::~Messenger() = default;
#else
Messenger::~Messenger() {
    assert(delegate_to_iterator_.empty());
}
#endif

// Check if the detectors match for the message and the delegate
static bool check_send(BaseMessage* message, BaseDelegate* delegate) {
    if(delegate->getDetector() != nullptr &&
       (message->getDetector() == nullptr || delegate->getDetector()->getName() != message->getDetector()->getName())) {
        return false;
    }
    return true;
}

/**
 * Messages should be bound during construction, so this function only gives useful information outside the constructor
 */
bool Messenger::hasReceiver(Module* source, const std::shared_ptr<BaseMessage>& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    const BaseMessage* inst = message.get();
    std::type_index type_idx = typeid(*inst);

    // Get the name of the output message
    std::string name = source->get_configuration().get<std::string>("output");

    // Check if a normal specific listener exists
    for(auto& delegate : delegates_[type_idx][name]) {
        if(check_send(message.get(), delegate.get())) {
            return true;
        }
    }
    // Check if a normal generic listener exists
    for(auto& delegate : delegates_[type_idx]["*"]) {
        if(check_send(message.get(), delegate.get())) {
            return true;
        }
    }
    // Check if a base message specific listener exists
    for(auto& delegate : delegates_[typeid(BaseMessage)][name]) {
        if(check_send(message.get(), delegate.get())) {
            return true;
        }
    }
    // Check if a base message generic listener exists
    for(auto& delegate : delegates_[typeid(BaseMessage)]["*"]) {
        if(check_send(message.get(), delegate.get())) {
            return true;
        }
    }

    return false;
}

void Messenger::add_delegate(const std::type_info& message_type,
                             Module* module,
                             const std::shared_ptr<BaseDelegate>& delegate) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Register generic or specific delegate depending on flag
    std::string message_name;
    if((delegate->getFlags() & MsgFlags::IGNORE_NAME) != MsgFlags::NONE) {
        message_name = "*";
    } else {
        message_name = module->get_configuration().get<std::string>("input");
    }

    // Register delegate internally
    delegates_[std::type_index(message_type)][message_name].push_back(delegate);
    auto delegate_iter = --delegates_[std::type_index(message_type)][message_name].end();
    delegate_to_iterator_.emplace(delegate_iter->get(),
                                  std::make_tuple(std::type_index(message_type), message_name, delegate_iter));

    // Add delegate to the module itself
    module->add_delegate(this, delegate_iter->get());
}

/**
 * @throws std::out_of_range If a delegate is removed which is never registered
 */
void Messenger::remove_delegate(BaseDelegate* delegate) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto iter = delegate_to_iterator_.find(delegate);
    if(iter == delegate_to_iterator_.end()) {
        throw std::out_of_range("delegate not found in listeners");
    }
    delegates_[std::get<0>(iter->second)][std::get<1>(iter->second)].erase(std::get<2>(iter->second));
    delegate_to_iterator_.erase(iter);
}
