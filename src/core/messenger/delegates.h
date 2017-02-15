/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_DELEGATE_H
#define ALLPIX_DELEGATE_H

#include <typeinfo>
#include <cassert>

#include "Message.hpp"
#include "../utils/exceptions.h"

namespace allpix{
    class BaseDelegate{
    public:
        BaseDelegate(){}
        virtual ~BaseDelegate(){}
        
        virtual void call(const Message &msg) const = 0;
    };

    template<typename T, typename R> class Delegate : public BaseDelegate{
    public:
        Delegate(T *obj, void (T::*method)(R)): obj_(obj), method_(method) {}
        ~Delegate(){}
        
        void call(const Message &msg) const{
            //the type names should have been correctly resolved earlier
            assert(typeid(msg) == typeid(R));
            
            //NOTE: this dynamic cast is not perfect, but otherwise dynamic linking will break
            (obj_->*method_)(dynamic_cast<const R&>(msg));
        }
    private:
        T *obj_;
        void (T::*method_)(R);
    };
}

#endif
