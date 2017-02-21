/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_DELEGATE_H
#define ALLPIX_DELEGATE_H

#include <typeinfo>
#include <memory>
#include <cassert>

#include "Message.hpp"
#include "../utils/exceptions.h"

namespace allpix {
    // base class for all delegates (template type erasure)
    class BaseDelegate {
    public:
        BaseDelegate() {}
        virtual ~BaseDelegate() {}
        
        virtual void call(std::shared_ptr<Message> msg) const = 0;
    };

    // delegate for receiver functions
    template<typename T, typename R> class Delegate : public BaseDelegate{
    public:
        using ListenerFunction = void (T::*)(std::shared_ptr<R>);
        
        Delegate(T *obj, ListenerFunction method): obj_(obj), method_(method) {}
        ~Delegate() {}
        
        void call(std::shared_ptr<Message> msg) const {
            // the type names should have been correctly resolved earlier
            assert(typeid(*msg) == typeid(R));
            
            // NOTE: this dynamic cast is not perfect, but otherwise dynamic linking will break
            (obj_->*method_)(std::dynamic_pointer_cast<R>(msg));
        }
    private:
        T *obj_;
        ListenerFunction method_;
    };
    
    // delegate for single bound shared pointer
    template<typename T, typename R> class SingleBindDelegate : public BaseDelegate{
    public:
        using BindType = std::shared_ptr<R> T::*;
        
        SingleBindDelegate(T *obj, BindType member): obj_(obj), member_(member) {}
        ~SingleBindDelegate() {}
        
        void call(std::shared_ptr<Message> msg) const {
            // the type names should have been correctly resolved earlier
            assert(typeid(*msg) == typeid(R));
            
            // FIXME: check that this assignment does not remove earlier information
            
            // NOTE: this dynamic cast is not perfect, but otherwise dynamic linking will break
            obj_->*member_ = std::dynamic_pointer_cast<R>(msg);
        }
    private:
        T *obj_;
        BindType member_;
    };
    
    // delegate for a vector of messages that can be received
    template<typename T, typename R> class VectorBindDelegate : public BaseDelegate{
    public:
        using BindType = std::vector<std::shared_ptr<R>> T::*;
        
        VectorBindDelegate(T *obj, BindType member): obj_(obj), member_(member) {}
        ~VectorBindDelegate() {}
        
        void call(std::shared_ptr<Message> msg) const {
            // the type names should have been correctly resolved earlier
            assert(typeid(*msg) == typeid(R));
            
            // NOTE: this dynamic cast is not perfect, but otherwise dynamic linking will break
            (obj_->*member_).push_back(std::dynamic_pointer_cast<R>(msg));
        }
    private:
        T *obj_;
        BindType member_;
        
    };
}

#endif /* ALLPIX_DELEGATE_H */
