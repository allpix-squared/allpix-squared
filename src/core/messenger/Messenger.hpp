/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MESSENGER_H
#define ALLPIX_MESSENGER_H

#include <vector>
#include <map>
#include <typeindex>

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
        // FIXME: is the empty string a proper catch-all or do we want to have a clearer separation?
        template <typename T, typename R> void registerListener(T *receiver, void (T::*method)(R), std::string message_type = "");
        
        // Dispatch message
        void dispatchMessage(const Message &msg, std::string name = "");
    private:
        using DelegateMap = std::map<std::type_index, std::map<std::string, std::vector<BaseDelegate*>>>;
        
        DelegateMap delegates_;
    };
    
    template <typename T, typename R> void Messenger::registerListener(T *receiver, void (T::*method)(R), std::string message_type){
        static_assert(std::is_base_of<Module, T>::value, "Receiver should be a module");
        static_assert(std::is_base_of<Message, R>::value, "Notifier method should take a message as argument");
        
        BaseDelegate *delegate = new Delegate<T, R>(receiver, method);
        delegates_[std::type_index(typeid(R))][message_type].push_back(delegate);
    }
}

#endif // ALLPIX_MESSENGER_H
