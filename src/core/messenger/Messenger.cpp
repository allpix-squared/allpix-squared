/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

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

// Constructor and destructor
Messenger::Messenger() : delegates_() {}
Messenger::~Messenger() = default;

// Check if we can send the message to this delegate
static bool check_send(BaseMessage* message, BaseDelegate* delegate) {
    // do checks (FIXME: this is really hidden away now...)
    if(delegate->getDetector() != nullptr &&
       (message->getDetector() == nullptr || delegate->getDetector()->getName() != message->getDetector()->getName())) {
        return false;
    }
    return true;
}

// Dispatch the message
void Messenger::dispatchMessage(const std::shared_ptr<BaseMessage>& msg, const std::string& name) {
    bool send = false;

    // create type identifier from typeid
    const BaseMessage* inst = msg.get();
    std::type_index type_idx = typeid(*inst);

    // send named messages only to their specific listeners
    if(!name.empty()) {
        for(auto& delegate : delegates_[type_idx][name]) {
            if(check_send(msg.get(), delegate.get())) {
                delegate->process(msg);
                send = true;
            }
        }
    }

    // send all messages also to general listeners
    for(auto& delegate : delegates_[type_idx][""]) {
        if(check_send(msg.get(), delegate.get())) {
            delegate->process(msg);
            send = true;
        }
    }

    if(!send) {
        LOG(WARNING) << "Dispatched message of type " << allpix::demangle(type_idx.name())
                     << " has no receivers... this is probably not what you want!";
    }
}

// Add a delegate (common register logic)
void Messenger::add_delegate(const std::type_info& message_type,
                             const std::string& message_name,
                             std::unique_ptr<BaseDelegate> delegate) {
    // add the delegate to the map
    delegates_[std::type_index(message_type)][message_name].push_back(std::move(delegate));
}
