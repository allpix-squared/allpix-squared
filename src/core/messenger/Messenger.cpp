/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "Messenger.hpp"

#include <string>

#include "../utils/log.h"
#include "../module/Module.hpp"
#include "Message.hpp"

using namespace allpix;

// Destructor
Messenger::~Messenger(){
    // delete all delegates
    for(auto &iter : delegates_){
        for(auto &del : iter.second){
            delete del;
        }
        iter.second.clear();
    }
}

// Dispatch message
// FIXME universal reference
void Messenger::dispatchMessage(std::string name, const Message &msg, Module *){
    if(delegates_[name].empty()){
        LOG(logWARNING) << "Dispatched message has no receivers... this is probably not what you want!";
    }
    
    for(const auto &del : delegates_[name]){
        del->call(msg);
    }
}
