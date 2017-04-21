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
    // Flags to pass to a receiver
    // FIXME: not the most logical location
    enum class MsgFlags : uint32_t {
        NONE = 0,                  // no enabled flags
        REQUIRED = (1 << 0),       // require a message before running a module
        NO_RESET = (1 << 1),       // do not reset a message after run
        ALLOW_OVERWRITE = (1 << 2) // allow overwriting a previous message
    };
    inline MsgFlags operator|(MsgFlags f1, MsgFlags f2) {
        return static_cast<MsgFlags>(static_cast<uint32_t>(f1) | static_cast<uint32_t>(f2));
    }
    inline MsgFlags operator&(MsgFlags f1, MsgFlags f2) {
        return static_cast<MsgFlags>(static_cast<uint32_t>(f1) & static_cast<uint32_t>(f2));
    }

    // Base class for all delegates (template type erasure)
    class BaseDelegate {
    public:
        // Constructor and destructor
        explicit BaseDelegate(MsgFlags flags) : processed_(false), flags_(flags) {}
        virtual ~BaseDelegate() = default;

        // Disallow copy
        BaseDelegate(const BaseDelegate&) = delete;
        BaseDelegate& operator=(const BaseDelegate&) = delete;

        // Check if delegate is satisfied (has been processed if required)
        bool isSatisfied() const {
            // always satisfied if not required
            if((getFlags() & MsgFlags::REQUIRED) == MsgFlags::NONE) {
                return true;
            }
            return processed_;
        }

        // Get the flags
        MsgFlags getFlags() const { return flags_; }

        // Get linked detector
        virtual std::shared_ptr<Detector> getDetector() const = 0;

        // Process the delegate
        virtual void process(std::shared_ptr<BaseMessage> msg) = 0;

        // Reset the delegate
        virtual void reset() {
            // Set processed to false again
            processed_ = false;
        }

    protected:
        void set_processed() { processed_ = true; }
        bool processed_;

        MsgFlags flags_;
    };

    // All delegates that need a Module
    template <typename T> class ModuleDelegate : public BaseDelegate {
    public:
        explicit ModuleDelegate(MsgFlags flags, T* obj) : BaseDelegate(flags), obj_(obj) {}
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

        FunctionDelegate(MsgFlags flags, T* obj, ListenerFunction method) : ModuleDelegate<T>(flags, obj), method_(method) {}
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

            // Pass the message and mark as processed
            (this->obj_->*method_)(std::static_pointer_cast<R>(msg));
            this->set_processed();
        }

    private:
        ListenerFunction method_;
    };

    // Delegate for single bound shared pointer
    template <typename T, typename R> class SingleBindDelegate : public ModuleDelegate<T> {
    public:
        using BindType = std::shared_ptr<R> T::*;

        SingleBindDelegate(MsgFlags flags, T* obj, BindType member) : ModuleDelegate<T>(flags, obj), member_(member) {}
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
            // Raise an error if the message is overwritten (unless it is allowed)
            if(this->obj_->*member_ != nullptr && (this->getFlags() & MsgFlags::ALLOW_OVERWRITE) == MsgFlags::NONE) {
                throw UnexpectedMessageException("module name", typeid(R));
            }

            // Set the message and mark as processed
            this->obj_->*member_ = std::static_pointer_cast<R>(msg);
            this->set_processed();
        }

        // Set the obj back to nullptr
        void reset() override {
            // Always do base reset
            BaseDelegate::reset();

            // Clear if needed
            if((this->getFlags() & MsgFlags::NO_RESET) == MsgFlags::NONE) {
                this->obj_->*member_ = nullptr;
            };
        }

    private:
        BindType member_;
    };

    // Delegate for a vector of messages that can be received
    template <typename T, typename R> class VectorBindDelegate : public ModuleDelegate<T> {
    public:
        using BindType = std::vector<std::shared_ptr<R>> T::*;

        VectorBindDelegate(MsgFlags flags, T* obj, BindType member) : ModuleDelegate<T>(flags, obj), member_(member) {}
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
            this->set_processed(); // FIXME: always mark as processed because it is an array?
        }

        // Clear the vector of received messages
        void reset() override {
            // Always do base reset
            BaseDelegate::reset();

            // Clear if needed
            if((this->getFlags() & MsgFlags::NO_RESET) == MsgFlags::NONE) {
                (this->obj_->*member_).clear();
            }
        }

    private:
        BindType member_;
    };
} // namespace allpix

#endif /* ALLPIX_DELEGATE_H */
