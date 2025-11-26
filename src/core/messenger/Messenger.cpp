/**
 * @file
 * @brief Implementation of messenger
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "Messenger.hpp"

#include <cassert>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <tuple>
#include <typeindex>
#include <typeinfo>
#include <utility>
#include <vector>

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
Messenger::~Messenger() { assert(delegate_to_iterator_.empty()); }
#endif

// Check if the detectors match for the message and the delegate and that we don't have self-dispatch
static bool check_send(Module* source, BaseMessage* message, BaseDelegate* delegate) {
    if(delegate->getDetector() != nullptr &&
       (message->getDetector() == nullptr || delegate->getDetector()->getName() != message->getDetector()->getName())) {
        return false;
    }
    if(delegate->getUniqueName() == source->getUniqueName()) {
        return false;
    }
    return true;
}

/**
 * Messages should be bound during construction, so this function only gives useful information outside the constructor
 */
bool Messenger::hasReceiver(Module* source, const std::shared_ptr<BaseMessage>& message) {
    std::lock_guard<std::mutex> const lock(mutex_);

    const BaseMessage* inst = message.get();
    std::type_index const type_idx = typeid(*inst);

    // Get the name of the output message
    auto name = source->get_configuration().get<std::string>("output");

    // Check if a normal specific listener exists
    for(auto& delegate : delegates_[type_idx][name]) {
        if(check_send(source, message.get(), delegate.get())) {
            return true;
        }
    }
    // Check if a normal generic listener exists
    for(auto& delegate : delegates_[type_idx]["*"]) {
        if(check_send(source, message.get(), delegate.get())) {
            return true;
        }
    }
    // Check if a base message specific listener exists
    for(auto& delegate : delegates_[typeid(BaseMessage)][name]) {
        if(check_send(source, message.get(), delegate.get())) {
            return true;
        }
    }
    // Check if a base message generic listener exists
    for(auto& delegate : delegates_[typeid(BaseMessage)]["*"]) {
        if(check_send(source, message.get(), delegate.get())) {
            return true;
        }
    }

    return false;
}

bool Messenger::isSatisfied(BaseDelegate* delegate, Event* event) {
    auto* local_messenger = event->get_local_messenger();
    return local_messenger->isSatisfied(delegate);
}

void Messenger::add_delegate(const std::type_info& message_type,
                             Module* module,
                             const std::shared_ptr<BaseDelegate>& delegate) {
    std::lock_guard<std::mutex> const lock(mutex_);

    // Register generic or specific delegate depending on flag
    std::string message_name;
    if((delegate->getFlags() & MsgFlags::IGNORE_NAME) != MsgFlags::NONE) {
        message_name = "*";
    } else if((delegate->getFlags() & MsgFlags::UNNAMED_ONLY) != MsgFlags::NONE) {
        message_name = "?";
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
 * @throws std::out_of_range if a delegate is removed which is never registered
 */
void Messenger::remove_delegate(BaseDelegate* delegate) {
    std::lock_guard<std::mutex> const lock(mutex_);

    auto iter = delegate_to_iterator_.find(delegate);
    if(iter == delegate_to_iterator_.end()) {
        throw std::out_of_range("delegate not found in listeners");
    }
    delegates_[std::get<0>(iter->second)][std::get<1>(iter->second)].erase(std::get<2>(iter->second));
    delegate_to_iterator_.erase(iter);
}

std::vector<std::pair<std::shared_ptr<BaseMessage>, std::string>> Messenger::fetchFilteredMessages(Module* module,
                                                                                                   Event* event) {
    try {
        auto* local_messenger = event->get_local_messenger();
        return local_messenger->fetchFilteredMessages(module);
    } catch(const std::out_of_range& e) {
        // No messages available after filtering, return empty vector:
        return {};
    }
}

LocalMessenger::LocalMessenger(Messenger& global_messenger) : global_messenger_(global_messenger) {}

void LocalMessenger::dispatchMessage(Module* source, std::shared_ptr<BaseMessage> message, std::string name) { // NOLINT
    // Get the name of the output message
    if(name == "-") {
        name = source->get_configuration().get<std::string>("output");
    }

    bool send = false;

    // Send messages to specific listeners
    send = dispatchMessage(source, message, name, name) || send;

    // Send to generic listeners
    send = dispatchMessage(source, message, name, "*") || send;

    // Send to listeners of unnamed messages
    if(name.empty()) {
        send = dispatchMessage(source, message, name, "?") || send;
    }

    // Display a TRACE log message if the message is send to no receiver
    if(!send) {
        const BaseMessage* inst = message.get();
        LOG(TRACE) << "Dispatched message " << allpix::demangle(typeid(*inst).name()) << " from " << source->getUniqueName()
                   << " has no receivers!";
    }

    // Save a copy of the sent message
    sent_messages_.emplace_back(message);
}

bool LocalMessenger::dispatchMessage(Module* source,
                                     const std::shared_ptr<BaseMessage>& message,
                                     const std::string& name,
                                     const std::string& id) {
    bool send = false;

    // Create type identifier from the typeid
    const BaseMessage* inst = message.get();
    std::type_index const type_idx = typeid(*inst);

    // Retrieve listeners for the given message type and name
    const auto msg_type_iterator = global_messenger_.delegates_.find(type_idx);
    if(msg_type_iterator != global_messenger_.delegates_.end()) {
        const auto msg_name_iterator = msg_type_iterator->second.find(id);
        if(msg_name_iterator != msg_type_iterator->second.end()) {
            // Send messages only to their specific listeners
            for(const auto& delegate : msg_name_iterator->second) {
                if(check_send(source, message.get(), delegate.get())) {
                    LOG(TRACE) << "Sending message " << allpix::demangle(type_idx.name()) << " from "
                               << source->getUniqueName() << " to " << delegate->getUniqueName();
                    // Construct BaseMessage where message should be stored
                    auto& dest = messages_[delegate->getUniqueName()][type_idx];

                    delegate->process(message, name, dest);
                    send = true;
                }
            }
        }
    }

    // Dispatch to base message listeners
    assert(typeid(BaseMessage) != typeid(*inst));
    const auto base_msg_type_iterator = global_messenger_.delegates_.find(typeid(BaseMessage));
    if(base_msg_type_iterator != global_messenger_.delegates_.end()) {
        const auto msg_name_iterator = base_msg_type_iterator->second.find(id);
        if(msg_name_iterator != base_msg_type_iterator->second.end()) {
            for(const auto& delegate : msg_name_iterator->second) {
                if(check_send(source, message.get(), delegate.get())) {
                    LOG(TRACE) << "Sending message " << allpix::demangle(type_idx.name()) << " from "
                               << source->getUniqueName() << " to generic listener " << delegate->getUniqueName();
                    auto& dest = messages_[delegate->getUniqueName()][typeid(BaseMessage)];
                    delegate->process(message, name, dest);
                    send = true;
                }
            }
        }
    }

    return send;
}

std::vector<std::pair<std::shared_ptr<BaseMessage>, std::string>> LocalMessenger::fetchFilteredMessages(Module* module) {
    const std::type_index type_idx = typeid(BaseMessage);
    return messages_.at(module->getUniqueName()).at(type_idx).filter_multi;
}

bool LocalMessenger::isSatisfied(BaseDelegate* delegate) const {
    // check our records for messages for this module
    const std::string name = delegate->getUniqueName();
    auto messages_iter = messages_.find(name);
    if(messages_iter == messages_.end()) {
        return false;
    }

    // check if this delegate is in our records
    auto delegate_iter = global_messenger_.delegate_to_iterator_.find(delegate);
    if(delegate_iter == global_messenger_.delegate_to_iterator_.end()) {
        throw std::out_of_range("delegate not found in listeners");
    }

    // check our records for messages for this delegate
    auto iter = messages_iter->second.find(std::get<0>(delegate_iter->second));
    return iter != messages_iter->second.end();
}
