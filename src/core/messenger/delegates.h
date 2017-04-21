/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_DELEGATE_H
#define ALLPIX_DELEGATE_H

#include <cassert>
#include <memory>
#include <typeinfo>

#include "Message.hpp"
#include "core/geometry/Detector.hpp"
#include "exceptions.h"

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

        // Get linked detector
        virtual std::shared_ptr<Detector> getDetector() const = 0;

        // Process the delegate
        virtual void process(std::shared_ptr<BaseMessage> msg) const = 0;
    };

    template <typename T> class ModuleDelegate : public BaseDelegate {
    public:
        explicit ModuleDelegate(T* obj) : obj_(obj) {}
        ~ModuleDelegate() override = default;

        // Disallow copy
        ModuleDelegate(const ModuleDelegate&) = delete;
        ModuleDelegate& operator=(const ModuleDelegate&) = delete;

        // Attached detector
        std::shared_ptr<Detector> getDetector() const override { return obj_->getDetector(); }

    protected:
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

        void process(std::shared_ptr<BaseMessage> msg) const override {
#ifndef NDEBUG
            // the type names should have been correctly resolved earlier
            const BaseMessage* inst = msg.get();
            assert(typeid(*inst) == typeid(R));
#endif

            // NOTE: this dynamic cast is not perfect, but otherwise dynamic linking will break
            (this->obj_->*method_)(std::static_pointer_cast<R>(msg));
        }

    private:
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

        void process(std::shared_ptr<BaseMessage> msg) const override {
#ifndef NDEBUG
            // the type names should have been correctly resolved earlier
            const BaseMessage* inst = msg.get();
            assert(typeid(*inst) == typeid(R));
#endif

            // FIXME: check that this assignment does not remove earlier information

            // NOTE: this dynamic cast is not perfect, but otherwise dynamic linking will break
            this->obj_->*member_ = std::static_pointer_cast<R>(msg);
        }

    private:
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

        void process(std::shared_ptr<BaseMessage> msg) const override {
#ifndef NDEBUG
            // the type names should have been correctly resolved earlier
            const BaseMessage* inst = msg.get();
            assert(typeid(*inst) == typeid(R));
#endif

            // NOTE: this dynamic cast is not perfect, but otherwise dynamic linking will break
            (this->obj_->*member_).push_back(std::static_pointer_cast<R>(msg));
        }

    private:
        BindType member_;
    };
} // namespace allpix

#endif /* ALLPIX_DELEGATE_H */
