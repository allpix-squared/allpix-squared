#include "Messenger.hpp"

#include <memory>
#include <string>
#include <typeindex>

#include "Message.hpp"
#include "core/module/Module.hpp"
#include "core/utils/log.h"
#include "core/utils/type.h"
#include "delegates.h"

using namespace allpix;

Messenger::Messenger() : delegates_() {}

// Check if the detectors match for the message and the delegate
static bool check_send(BaseMessage* message, BaseDelegate* delegate) {
    if(delegate->getDetector() != nullptr &&
       (message->getDetector() == nullptr || delegate->getDetector()->getName() != message->getDetector()->getName())) {
        return false;
    }
    return true;
}

/**
 * Messages are only dispatched to delegates listening to the exact same type. If the dispatched message
 * has no name it only sends to all general listeners (not listening to a specific name). If the dispatched
 * message has a name it is also distributed to its specific listeners (besides the general listeners).
 */
void Messenger::dispatch_message(const std::shared_ptr<BaseMessage>& msg, const std::string& name) {
    bool send = false;

    // Create type identifier from the typeid
    const BaseMessage* inst = msg.get();
    std::type_index type_idx = typeid(*inst);

    // Send named messages only to their specific listeners
    if(!name.empty()) {
        for(auto& delegate : delegates_[type_idx][name]) {
            if(check_send(msg.get(), delegate.get())) {
                delegate->process(msg);
                send = true;
            }
        }
    }

    // Send all messages also to general listeners
    for(auto& delegate : delegates_[type_idx][""]) {
        if(check_send(msg.get(), delegate.get())) {
            delegate->process(msg);
            send = true;
        }
    }

    // Display a warning if the message is send to no receiver
    // FIXME: better message about source (and check if this really a problem)
    if(!send) {
        LOG(WARNING) << "Dispatched message of type " << allpix::demangle(type_idx.name())
                     << " has no receivers... this is probably not what you want!";
    }
}

// Add a delegate (common register logic)
void Messenger::add_delegate(const std::type_info& message_type,
                             const std::string& message_name,
                             std::unique_ptr<BaseDelegate> delegate) {
    delegates_[std::type_index(message_type)][message_name].push_back(std::move(delegate));
}
