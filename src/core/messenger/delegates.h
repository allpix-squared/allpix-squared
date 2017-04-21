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

namespace allpix {
    // Base class for all delegates (template type erasure)
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
        virtual void process(std::shared_ptr<BaseMessage> msg) = 0;

        // Reset the delegate
        virtual void reset() = 0;
    };

    // All delegates that need a Module
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

    // Delegate for receiver functions
    template <typename T, typename R> class FunctionDelegate : public ModuleDelegate<T> {
    public:
        using ListenerFunction = void (T::*)(std::shared_ptr<R>);

        FunctionDelegate(T* obj, ListenerFunction method) : ModuleDelegate<T>(obj), method_(method) {}
        ~FunctionDelegate() override = default;

        // Disallow copy
        FunctionDelegate(const FunctionDelegate&) = delete;
        FunctionDelegate& operator=(const FunctionDelegate&) = delete;

        // Send the message to the function
        void process(std::shared_ptr<BaseMessage> msg) override {
#ifndef NDEBUG
            // The type names should have been correctly resolved earlier
            const BaseMessage* inst = msg.get();
            assert(typeid(*inst) == typeid(R));
#endif

            // Pass the message
            (this->obj_->*method_)(std::static_pointer_cast<R>(msg));
        }

        // Reset is not needed
        void reset() override {}

    private:
        ListenerFunction method_;
    };

    // Delegate for single bound shared pointer
    template <typename T, typename R> class SingleBindDelegate : public ModuleDelegate<T> {
    public:
        using BindType = std::shared_ptr<R> T::*;

        SingleBindDelegate(T* obj, BindType member) : ModuleDelegate<T>(obj), member_(member) {}
        ~SingleBindDelegate() override = default;

        // Disallow copy
        SingleBindDelegate(const SingleBindDelegate&) = delete;
        SingleBindDelegate& operator=(const SingleBindDelegate&) = delete;

        // Set the member to the supplied message
        void process(std::shared_ptr<BaseMessage> msg) override {
#ifndef NDEBUG
            // The type names should have been correctly resolved earlier
            const BaseMessage* inst = msg.get();
            assert(typeid(*inst) == typeid(R));
#endif

            // FIXME: check that this assignment does not remove earlier information

            // Set the message
            this->obj_->*member_ = std::static_pointer_cast<R>(msg);
        }

        // Set the obj back to nullptr
        void reset() override { this->obj_->*member_ = nullptr; }

    private:
        BindType member_;
    };

    // Delegate for a vector of messages that can be received
    template <typename T, typename R> class VectorBindDelegate : public ModuleDelegate<T> {
    public:
        using BindType = std::vector<std::shared_ptr<R>> T::*;

        VectorBindDelegate(T* obj, BindType member) : ModuleDelegate<T>(obj), member_(member) {}
        ~VectorBindDelegate() override = default;

        // Disallow copy
        VectorBindDelegate(const VectorBindDelegate&) = delete;
        VectorBindDelegate& operator=(const VectorBindDelegate&) = delete;

        void process(std::shared_ptr<BaseMessage> msg) override {
#ifndef NDEBUG
            // The type names should have been correctly resolved earlier
            const BaseMessage* inst = msg.get();
            assert(typeid(*inst) == typeid(R));
#endif
            // Add the message
            (this->obj_->*member_).push_back(std::static_pointer_cast<R>(msg));
        }

        // Clear the vector of received messages
        void reset() override { (this->obj_->*member_).clear(); }

    private:
        BindType member_;
    };
} // namespace allpix

#endif /* ALLPIX_DELEGATE_H */
