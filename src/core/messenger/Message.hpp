/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MESSAGE_H
#define ALLPIX_MESSAGE_H

namespace allpix{
    class Message{
    public:
        // Constructor and destructors
        Message() {}
        virtual ~Message() {}
        
        // Disallow copy
        Message(const Message&) = delete;
        Message &operator=(const Message&) = delete;
        
        // TODO: we might want to have a resolving function to get the content of a message by name for histogramming etc.
    };
}

#endif // ALLPIX_MESSAGE_H
