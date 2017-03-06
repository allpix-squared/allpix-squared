/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "Messenger.hpp"

#include <string>

#include "../utils/type.h"
#include "../utils/log.h"

#include "../module/Module.hpp"
#include "Message.hpp"

using namespace allpix;

// Constructor
Messenger::Messenger(): delegates_() {}

// Destructor
Messenger::~Messenger() {
    // delete all delegates
    for (auto &type_delegates : delegates_) {
        for (auto &name_delegates : type_delegates.second) {
            for (auto &delegate : name_delegates.second) {
                delete delegate;
            }
        }
    }
}

// Dispatch the message
void Messenger::dispatchMessage(std::shared_ptr<Message> msg, std::string name) {
    bool send = false;
        
    // NOTE: we are not sending messages with unspecified names to everyone listening
    if (!name.empty()) {
        for (auto &delegate : delegates_[typeid(*msg)][name]) {
            delegate->call(msg);
            send = true;
        }
    }
    
    // NOTE: we do send all messages also to general listeners
    for (auto &delegate : delegates_[typeid(*msg)][""]) {
        delegate->call(msg);
        send = true;
    }
    
    if (!send) {
        LOG(WARNING) << "Dispatched message of type " << allpix::demangle(typeid(*msg).name()) << " has no receivers... this is probably not what you want!";
    }
}
