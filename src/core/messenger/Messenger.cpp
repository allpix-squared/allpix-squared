/**
 * @file
 * @brief Implementation of messenger
 *
 * @copyright MIT License
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
    assert(delegates_.empty() && delegate_to_iterator_.empty());
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
bool Messenger::hasReceiver(Module* module, const std::shared_ptr<BaseMessage>& message) {
    const BaseMessage* inst = message.get();
    std::type_index type_idx = typeid(*inst);

    // Get the name of the output message
    std::string name = module->get_configuration().get<std::string>("output");

    // Check if a listener exists
    for(auto& delegate : delegates_[type_idx][name]) {
        if(check_send(message.get(), delegate.get())) {
            return true;
        }
    }

    return false;
}

/**
 * Messages are only dispatched to delegates listening to the exact same type. If the dispatched message
 * has no name it only sends to all general listeners (not listening to a specific name). If the dispatched
 * message has a name it is also distributed to its specific listeners (besides the general listeners).
 */
void Messenger::dispatch_message(Module* module, const std::shared_ptr<BaseMessage>& message) {
    bool send = false;

    // Get the name of the output message
    std::string name = module->get_configuration().get<std::string>("output");

    // Create type identifier from the typeid
    const BaseMessage* inst = message.get();
    std::type_index type_idx = typeid(*inst);

    // Send messages only to their specific listeners
    for(auto& delegate : delegates_[type_idx][name]) {
        if(check_send(message.get(), delegate.get())) {
            LOG(TRACE) << "Sending message " << allpix::demangle(type_idx.name()) << " from " << module->getUniqueName()
                       << " to " << delegate->getUniqueName();
            delegate->process(message);
            send = true;
        }
    }

    // Display a warning if the message is send to no receiver
    // FIXME: Check better if this is a real problem (or do this always only in the module)
    if(!send) {
        LOG(WARNING) << "Dispatched message " << allpix::demangle(type_idx.name()) << " from " << module->getUniqueName()
                     << " has no receivers!";
    }
}

void Messenger::add_delegate(const std::type_info& message_type, Module* module, std::unique_ptr<BaseDelegate> delegate) {
    std::string message_name = module->get_configuration().get<std::string>("input");

    // Register delegate internally
    delegates_[std::type_index(message_type)][message_name].push_back(std::move(delegate));
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
    auto iter = delegate_to_iterator_.find(delegate);
    if(iter == delegate_to_iterator_.end()) {
        throw std::out_of_range("delegate not found in listeners");
    }
    delegates_[std::get<0>(iter->second)][std::get<1>(iter->second)].erase(std::get<2>(iter->second));
    delegate_to_iterator_.erase(iter);
}
