/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MESSENGER_H
#define ALLPIX_MESSENGER_H

#include <vector>
#include <map>

#include "delegates.h"
#include "../module/Module.hpp"
#include "Message.hpp"

namespace allpix{

    class Messenger{
    public:
        // Constructor and destructors
        Messenger() {}
        virtual ~Messenger();
        
        // Disallow copy
        Messenger(const Messenger&) = delete;
        Messenger &operator=(const Messenger&) = delete;
        
        // Register a listener
        template <typename T, typename R> void registerListener(T *receiver, void (T::*method)(R), std::string message_type);
        
        // Dispatch message
        void dispatchMessage(std::string name, const Message &msg, Module *source = 0);
    private:
        using DelegateMap = std::map<std::string, std::vector<BaseDelegate*> >;
        
        DelegateMap delegates_;
    };
    
    template <typename T, typename R> void Messenger::registerListener(T *receiver, void (T::*method)(R), std::string message_type){
        static_assert(std::is_base_of<Module, T>::value, "Receiver should be a module");
        static_assert(std::is_base_of<Message, R>::value, "Notifier method should take a message as argument");
        
        BaseDelegate *delegate = new Delegate<T, R>(receiver, method);
        //FIXME: add a check if previous delegates of same name take the same message
        //if(!delegates_[message_type].empty() && typeid(delegates_[message_type].front()) != typeid(R)
        delegates_[message_type].push_back(delegate);
    }
}

#endif // ALLPIX_MESSENGER_H
