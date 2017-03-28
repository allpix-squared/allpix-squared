/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "Messenger.hpp"

#include <memory>
#include <string>
#include <typeindex>

#include "core/module/Module.hpp"
#include "core/utils/log.h"
#include "core/utils/type.h"

#include "Message.hpp"

using namespace allpix;

// Constructor and destructor
Messenger::Messenger() : delegates_() {}
Messenger::~Messenger() = default;

// Dispatch the message
void Messenger::dispatchMessage(const std::shared_ptr<Message>& msg, const std::string& name) {
    bool send = false;

    // create type identifier from typeid
    const Message* inst = msg.get();
    std::type_index type_idx = typeid(*inst);

    // NOTE: we are not sending messages with unspecified names to everyone listening
    if(!name.empty()) {
        for(auto& delegate : delegates_[type_idx][name]) {
            delegate->call(msg);
            send = true;
        }
    }

    // NOTE: we do send all messages also to general listeners
    for(auto& delegate : delegates_[type_idx][""]) {
        delegate->call(msg);
        send = true;
    }

    if(!send) {
        LOG(WARNING) << "Dispatched message of type " << allpix::demangle(type_idx.name())
                     << " has no receivers... this is probably not what you want!";
    }
}
