/**
 * @file
 * @brief List of internal messenger delegates
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

/**
 * @defgroup Delegates Delegate classes
 * @brief  Collection of delegates serving as interface between messages and their receivers
 */

#ifndef ALLPIX_DELEGATE_H
#define ALLPIX_DELEGATE_H

#include <cassert>
#include <memory>
#include <typeinfo>

#include "Message.hpp"
#include "core/geometry/Detector.hpp"
#include "exceptions.h"

// TODO [doc] This should partly move to a source file

namespace allpix {
    /**
     * @ingroup Delegates
     * @brief Flags to change the behaviour of delegates
     *
     * All flags are distinct and can be combined using the | (OR) operator. The flags should be passed to the
     * \ref Messenger when \ref Messenger::registerListener "registering" a listener or when binding either a \ref
     * Messenger::bindSingle "single" or \ref Messenger::bindMulti "multiple" messages. It depends on the delegate which
     * combination is flags is valid.
     */
    // TODO [doc] Is DelegateFlags or MessengerFlags a better name (and in separate file?)
    enum class MsgFlags : uint32_t {
        NONE = 0,                   ///< No enabled flags
        REQUIRED = (1 << 0),        ///< Require a message before running a module
        ALLOW_OVERWRITE = (1 << 1), ///< Allow overwriting a previous message
        IGNORE_NAME = (1 << 2)      ///< Listen to all ignoring message name (equal to * as a input configuration parameter)
    };
    /**
     * @ingroup Delegates
     * @brief Combine two sets of message flags
     * @param f1 First set of flag
     * @param f2 Second set of flag
     * @return New set of flag representing the combination of the two given sets
     */
    inline MsgFlags operator|(MsgFlags f1, MsgFlags f2) {
        return static_cast<MsgFlags>(static_cast<uint32_t>(f1) | static_cast<uint32_t>(f2));
    }
    /**
     * @ingroup Delegates
     * @brief Give the set of flags present in both sets of message flags
     * @param f1 First flag
     * @param f2 Second flag
     * @return New flag representing the flags present in both sets of flags
     */
    inline MsgFlags operator&(MsgFlags f1, MsgFlags f2) {
        return static_cast<MsgFlags>(static_cast<uint32_t>(f1) & static_cast<uint32_t>(f2));
    }

    /**
     * @ingroup Delegates
     * @brief Base for all delegates
     *
     * The base class is used as type-erasure for its subclasses
     */
    class BaseDelegate {
    public:
        /**
         * @brief Construct a delegate with the supplied flags
         * @param flags Configuration flags
         */
        explicit BaseDelegate(MsgFlags flags) : processed_(false), flags_(flags) {}
        /**
         * @brief Essential virtual destructor
         */
        virtual ~BaseDelegate() = default;

        /// @{
        /**
         * @brief Copying a delegate is not allowed
         */
        BaseDelegate(const BaseDelegate&) = delete;
        BaseDelegate& operator=(const BaseDelegate&) = delete;
        /// @}

        /// @{
        /**
         * @brief Enable default move behaviour
         */
        BaseDelegate(BaseDelegate&&) noexcept = default;
        BaseDelegate& operator=(BaseDelegate&&) noexcept = default;
        /// @}

        /**
         * @brief Check if delegate satisfied its requirements
         * @return True if satisfied, false otherwise
         *
         * The delegate is always satisfied if the \ref MsgFlags::REQUIRED "REQUIRED" flag has not been passed. Otherwise it
         * is up to the subclasses to determine if a delegate has been processed.
         */
        bool isSatisfied() const {
            if((getFlags() & MsgFlags::REQUIRED) == MsgFlags::NONE) {
                return true;
            }
            return processed_;
        }

        /**
         * @brief Get the flags for this delegate
         * @return Message flags
         */
        MsgFlags getFlags() const { return flags_; }

        /**
         * @brief Get the detector bound to a delegate
         * @return Linked detector
         */
        virtual std::shared_ptr<Detector> getDetector() const = 0;

        /**
         * @brief Get the unique identifier for the bound object
         * @return Unique identifier
         */
        virtual std::string getUniqueName() = 0;

        /**
         * @brief Process a message and forwards it to its final destination
         * @param msg Message to process
         * @param name Name of the message
         */
        virtual void process(std::shared_ptr<BaseMessage> msg, std::string name) = 0;

        /**
         * @brief Reset the delegate and set it not satisfied again
         */
        virtual void reset() { processed_ = false; }

    protected:
        /**
         * @brief Set the processed flag to signal that the delegate is satisfied
         */
        void set_processed() { processed_ = true; }
        bool processed_;

        MsgFlags flags_;
    };

    /**
     * @ingroup Delegates
     * @brief Base for all delegates operating on modules
     *
     * As all delegates currently operate on modules (as messages should be dispatched to modules), this class is in fact the
     * templated base class of all delegates.
     */
    template <typename T> class ModuleDelegate : public BaseDelegate {
    public:
        /**
         * @brief Construct a module delegate for the given module
         * @param flags Messenger flags
         * @param obj Module object this delegate should operate on
         */
        explicit ModuleDelegate(MsgFlags flags, T* obj) : BaseDelegate(flags), obj_(obj) {}

        /**
         * @brief Get the unique name of the bound module
         * @return Unique name
         */
        std::string getUniqueName() override { return obj_->getUniqueName(); }

        /**
         * @brief Get the detector bound to this module
         *
         * Returns the bound detector for detector modules and a null pointer for unique modules
         */
        std::shared_ptr<Detector> getDetector() const override { return obj_->getDetector(); }

    protected:
        T* obj_;
    };

    /**
     * @ingroup Delegates
     * @brief Delegate to store the message in memory for fetching the history
     */
    template <typename T> class StoreDelegate : public ModuleDelegate<T> {
    public:
        /**
         * @brief Construct a function delegate for the given module
         * @param flags Messenger flags (note that REQUIRED does not mean all related objects are fetched)
         * @param obj Module object this delegate should store the message for
         */
        StoreDelegate(MsgFlags flags, T* obj) : ModuleDelegate<T>(flags, obj) {}

        /**
         * @brief Stores the received message in the delegate until the end of the event
         * @param msg Message to store
         */
        void process(std::shared_ptr<BaseMessage> msg, std::string) override {
            // Store the message and mark as processed
            messages_.push_back(msg);
            this->set_processed();
        }

        /**
         * @brief Reset the delegate by clearing the list of stored messages
         *
         * Always calls the BaseDelegate::reset first. Clears the storage of the messages
         */
        void reset() override {
            // Always do base reset
            BaseDelegate::reset();
            // Clear
            messages_.clear();
        }

    private:
        std::vector<std::shared_ptr<BaseMessage>> messages_;
    };

    /**
     * @ingroup Delegates
     * @brief Delegate for invoking a function in the module
     */
    template <typename T, typename R> class FunctionDelegate : public ModuleDelegate<T> {
    public:
        using ListenerFunction = void (T::*)(std::shared_ptr<R>);

        /**
         * @brief Construct a function delegate for the given module
         * @param flags Messenger flags
         * @param obj Module object this delegate should operate on
         * @param method A function taking a shared_ptr to the message to listen to
         */
        FunctionDelegate(MsgFlags flags, T* obj, ListenerFunction method) : ModuleDelegate<T>(flags, obj), method_(method) {}

        /**
         * @brief Calls the listener function with the supplied message
         * @param msg Message to process
         * @warning The listener function is called directly from the delegate, no heavy processing should be done in the
         *          listener function
         */
        void process(std::shared_ptr<BaseMessage> msg, std::string) override {
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

    /**
     * @ingroup Delegates
     * @brief Delegate for invoking a function listening to all messages also getting the name
     */
    template <typename T> class FunctionAllDelegate : public ModuleDelegate<T> {
    public:
        using ListenerFunction = void (T::*)(std::shared_ptr<BaseMessage>, std::string);

        /**
         * @brief Construct a function delegate for the given module
         * @param flags Messenger flags
         * @param obj Module object this delegate should operate on
         * @param method A function taking a shared_ptr to the message to listen to
         */
        FunctionAllDelegate(MsgFlags flags, T* obj, ListenerFunction method)
            : ModuleDelegate<T>(flags, obj), method_(method) {}

        /**
         * @brief Calls the listener function with the supplied message
         * @param msg Message to process
         * @param name Name of the message to process
         * @warning The listener function is called directly from the delegate, no heavy processing should be done in the
         *          listener function
         */
        void process(std::shared_ptr<BaseMessage> msg, std::string name) override {
            // Pass the message and mark as processed
            (this->obj_->*method_)(std::static_pointer_cast<BaseMessage>(msg), name);
            this->set_processed();
        }

    private:
        ListenerFunction method_;
    };

    /**
     * @ingroup Delegates
     * @brief Delegate for binding a single message
     */
    template <typename T, typename R> class SingleBindDelegate : public ModuleDelegate<T> {
    public:
        using BindType = std::shared_ptr<R> T::*;

        /**
         * @brief Construct a single bound delegate for the given module
         * @param flags Messenger flags
         * @param obj Module object this delegate should operate on
         * @param member Member variable to assign the pointer to the message to
         */
        SingleBindDelegate(MsgFlags flags, T* obj, BindType member) : ModuleDelegate<T>(flags, obj), member_(member) {}

        /**
         * @brief Saves the message to the bound message pointer
         * @param msg Message to process
         * @throws UnexpectedMessageException If this delegate has already received the message after the previous reset (not
         *         thrown if the \ref MsgFlags::ALLOW_OVERWRITE "ALLOW_OVERWRITE" flag is passed)
         *
         * The saved value is overwritten if the \ref MsgFlags::ALLOW_OVERWRITE "ALLOW_OVERWRITE" flag is enabled.
         */
        void process(std::shared_ptr<BaseMessage> msg, std::string) override {
#ifndef NDEBUG
            // The type names should have been correctly resolved earlier
            const BaseMessage* inst = msg.get();
            assert(typeid(*inst) == typeid(R));
#endif
            // Raise an error if the message is overwritten (unless it is allowed)
            if(this->obj_->*member_ != nullptr && (this->getFlags() & MsgFlags::ALLOW_OVERWRITE) == MsgFlags::NONE) {
                throw UnexpectedMessageException(this->obj_->getUniqueName(), typeid(R));
            }

            // Set the message and mark as processed
            this->obj_->*member_ = std::static_pointer_cast<R>(msg);
            this->set_processed();
        }

        /**
         * @brief Reset the delegate by resetting the bound variable
         *
         * Always calls the BaseDelegate::reset first. Set the referenced member to a null pointer
         */
        void reset() override {
            // Always do base reset
            BaseDelegate::reset();

            // Clear
            this->obj_->*member_ = nullptr;
        }

    private:
        BindType member_;
    };

    /**
     * @ingroup Delegates
     * @brief Delegate for binding multiple message to a vector
     */
    template <typename T, typename R> class VectorBindDelegate : public ModuleDelegate<T> {
    public:
        using BindType = std::vector<std::shared_ptr<R>> T::*;

        /**
         * @brief Construct a vector bound delegate for the given module
         * @param flags Messenger flags
         * @param obj Module object this delegate should operate on
         * @param member Member variable vector to add the pointer to the message to
         */
        VectorBindDelegate(MsgFlags flags, T* obj, BindType member) : ModuleDelegate<T>(flags, obj), member_(member) {}

        /**
         * @brief Adds the message to the bound vector
         * @param msg Message to process
         */
        void process(std::shared_ptr<BaseMessage> msg, std::string) override {
#ifndef NDEBUG
            // The type names should have been correctly resolved earlier
            const BaseMessage* inst = msg.get();
            assert(typeid(*inst) == typeid(R));
#endif
            // Add the message
            (this->obj_->*member_).push_back(std::static_pointer_cast<R>(msg));
            this->set_processed();
        }

        /**
         * @brief Reset the delegate by clearing the vector of messages
         *
         * Always calls the BaseDelegate::reset first. Clears the referenced vector to an empty state         */
        void reset() override {
            // Always do base reset
            BaseDelegate::reset();

            // Clear
            (this->obj_->*member_).clear();
        }

    private:
        BindType member_;
    };
} // namespace allpix

#endif /* ALLPIX_DELEGATE_H */
