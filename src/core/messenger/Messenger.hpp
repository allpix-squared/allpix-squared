/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MESSENGER_H
#define ALLPIX_MESSENGER_H

#include <vector>

#include "../module/Module.hpp"
#include "Message.hpp"

namespace allpix{

    class Messenger{
    public:
        // Constructor and destructors
        Messenger() {}
        virtual ~Messenger() {}
        
        // Disallow copy
        Messenger(const Messenger&) = delete;
        Messenger &operator=(const Messenger&) = delete;
        
        // Register a listener
        // FIXME: probably use a template here to abstract the type away (else we need a dynamic cast everywhere in the separate algorithms)
        void registerListener(Module *receiver, void *function_pointer_to_message_type, std::string message_type);
        // Dispatch message
        void dispatchMessage(std::string name, Message *msg, Module *source = 0);
    };
}

#endif // ALLPIX_MESSENGER_H
