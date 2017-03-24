/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_DELEGATE_H
#define ALLPIX_DELEGATE_H

#include <cassert>
#include <memory>
#include <typeinfo>

#include "Message.hpp"
#include "core/utils/exceptions.h"

// FIXME: RESTRUCTURE THIS A BIT TO BE MORE USABLE AND CONCEPTUALLY BETTER

namespace allpix {
    // base class for all delegates (template type erasure)
    class BaseDelegate {
    public:
        // Constructor and destructor
        BaseDelegate() = default;
        virtual ~BaseDelegate() = default;

        // Disallow copy
        BaseDelegate(const BaseDelegate&) = delete;
        BaseDelegate& operator=(const BaseDelegate&) = delete;

        // Execute the delegate
        virtual void call(std::shared_ptr<Message> msg) const = 0;
    };

    template <typename T> class ModuleDelegate : public BaseDelegate {
    public:
        explicit ModuleDelegate(T* obj) : obj_(obj) {}
        ~ModuleDelegate() override = default;

        // Disallow copy
        ModuleDelegate(const ModuleDelegate&) = delete;
        ModuleDelegate& operator=(const ModuleDelegate&) = delete;

        void call(std::shared_ptr<Message> msg) const override {
            // do checks (FIXME: this is really hidden away now...)
            if(obj_->getDetector() != nullptr) {
                if(msg->getDetector() != nullptr && obj_->getDetector()->getName() == msg->getDetector()->getName()) {
                    execute(msg);
                }
                return;
            }
            execute(msg);
        }

    protected:
        virtual void execute(std::shared_ptr<Message> msg) const = 0;

        T* obj_;
    };

    // delegate for receiver functions
    template <typename T, typename R> class Delegate : public ModuleDelegate<T> {
    public:
        using ListenerFunction = void (T::*)(std::shared_ptr<R>);

        Delegate(T* obj, ListenerFunction method) : ModuleDelegate<T>(obj), method_(method) {}
        ~Delegate() override = default;

        // Disallow copy
        Delegate(const Delegate&) = delete;
        Delegate& operator=(const Delegate&) = delete;

    private:
        void execute(std::shared_ptr<Message> msg) const override {
#ifndef NDEBUG
            // the type names should have been correctly resolved earlier
            const Message* inst = msg.get();
            assert(typeid(*inst) == typeid(R));
#endif

            // NOTE: this dynamic cast is not perfect, but otherwise dynamic linking will break
            (this->obj_->*method_)(std::static_pointer_cast<R>(msg));
        }

        ListenerFunction method_;
    };

    // delegate for single bound shared pointer
    template <typename T, typename R> class SingleBindDelegate : public ModuleDelegate<T> {
    public:
        using BindType = std::shared_ptr<R> T::*;

        SingleBindDelegate(T* obj, BindType member) : ModuleDelegate<T>(obj), member_(member) {}
        ~SingleBindDelegate() override = default;

        // Disallow copy
        SingleBindDelegate(const SingleBindDelegate&) = delete;
        SingleBindDelegate& operator=(const SingleBindDelegate&) = delete;

    private:
        void execute(std::shared_ptr<Message> msg) const override {
#ifndef NDEBUG
            // the type names should have been correctly resolved earlier
            const Message* inst = msg.get();
            assert(typeid(*inst) == typeid(R));
#endif

            // FIXME: check that this assignment does not remove earlier information

            // NOTE: this dynamic cast is not perfect, but otherwise dynamic linking will break
            this->obj_->*member_ = std::static_pointer_cast<R>(msg);
        }

        BindType member_;
    };

    // delegate for a vector of messages that can be received
    template <typename T, typename R> class VectorBindDelegate : public ModuleDelegate<T> {
    public:
        using BindType = std::vector<std::shared_ptr<R>> T::*;

        VectorBindDelegate(T* obj, BindType member) : ModuleDelegate<T>(obj), member_(member) {}
        ~VectorBindDelegate() override = default;

        // Disallow copy
        VectorBindDelegate(const VectorBindDelegate&) = delete;
        VectorBindDelegate& operator=(const VectorBindDelegate&) = delete;

    private:
        void execute(std::shared_ptr<Message> msg) const override {
#ifndef NDEBUG
            // the type names should have been correctly resolved earlier
            const Message* inst = msg.get();
            assert(typeid(*inst) == typeid(R));
#endif

            // NOTE: this dynamic cast is not perfect, but otherwise dynamic linking will break
            (this->obj_->*member_).push_back(std::static_pointer_cast<R>(msg));
        }

        BindType member_;
    };
}

#endif /* ALLPIX_DELEGATE_H */
