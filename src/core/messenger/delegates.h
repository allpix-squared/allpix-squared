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
    class BaseDelegate {
    public:
        BaseDelegate() {}
        virtual ~BaseDelegate() {}
        
        virtual void call(std::shared_ptr<Message> msg) const = 0;
    };

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
}

#endif /* ALLPIX_DELEGATE_H */
