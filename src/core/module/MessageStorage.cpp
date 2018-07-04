#include "MessageStorage.hpp"

using namespace allpix;

MessageStorage::MessageStorage(DelegateMap& delegates) : delegates_(delegates) {}

// Check if the detectors match for the message and the delegate
static bool check_send(BaseMessage* message, BaseDelegate* delegate) {
    if(delegate->getDetector() != nullptr &&
       (message->getDetector() == nullptr || delegate->getDetector()->getName() != message->getDetector()->getName())) {
        return false;
    }
    return true;
}

void MessageStorage::dispatch_message(Module* source, std::shared_ptr<BaseMessage> message, std::string name) {
    // Get the name of the output message
    if(name == "-") {
        name = source->get_configuration().get<std::string>("output");
    }

    bool send = false;

    // Send messages to specific listeners
    send = dispatch_message(source, message, name, name) || send;

    // Send to generic listeners
    send = dispatch_message(source, message, name, "*") || send;

    // Display a TRACE log message if the message is send to no receiver
    if(!send) {
        const BaseMessage* inst = message.get();
        LOG(TRACE) << "Dispatched message " << allpix::demangle(typeid(*inst).name()) << " from " << source->getUniqueName()
                   << " has no receivers!";
    }

    // Save a copy of the sent message
    sent_messages_.emplace_back(message);
}

bool MessageStorage::dispatch_message(Module* source,
                                      const std::shared_ptr<BaseMessage>& message,
                                      const std::string& name,
                                      const std::string& id) {
    bool send = false;

    // Create type identifier from the typeid
    const BaseMessage* inst = message.get();
    std::type_index type_idx = typeid(*inst);

    // Send messages only to their specific listeners
    for(auto& delegate : delegates_[type_idx][id]) {
        if(check_send(message.get(), delegate.get())) {
            LOG(TRACE) << "Sending message " << allpix::demangle(type_idx.name()) << " from " << source->getUniqueName()
                       << " to " << delegate->getUniqueName();
            // Construct BaseMessage where message should be stored
            auto& dest = messages_[delegate->getUniqueName()];

            delegate->process(message, name, dest);
            satisfied_modules_[delegate->getUniqueName()] = true;
            send = true;
        }
    }

    // Dispatch to base message listeners
    assert(typeid(BaseMessage) != typeid(*inst));
    for(auto& delegate : delegates_[typeid(BaseMessage)][id]) {
        if(check_send(message.get(), delegate.get())) {
            LOG(TRACE) << "Sending message " << allpix::demangle(type_idx.name()) << " from " << source->getUniqueName()
                       << " to generic listener " << delegate->getUniqueName();
            auto& dest = messages_[delegate->getUniqueName()];
            delegate->process(message, name, dest);
            satisfied_modules_[delegate->getUniqueName()] = true;
            send = true;
        }
    }

    return send;
}

bool MessageStorage::is_satisfied(Module* module) const {
    // Check delegate flags. If false, check event-local satisfaction.
    if(module->check_delegates()) {
        return true;
    }

    try {
        return satisfied_modules_.at(module->getUniqueName());
    } catch(const std::out_of_range&) {
        return false;
    }
}

MessageStorage& MessageStorage::using_module(Module* module) {
    module_ = module;
    return *this;
}
