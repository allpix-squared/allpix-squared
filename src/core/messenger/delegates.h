/**
 * @file
 * @brief List of internal messenger delegates
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

/**
 * @defgroup Delegates Delegate classes
 * @brief Collection of delegates serving as interface between messages and their receivers
 */

#ifndef ALLPIX_DELEGATE_H
#define ALLPIX_DELEGATE_H

#include <cassert>
#include <memory>
#include <typeinfo>
#include <utility>

#include "Message.hpp"
#include "core/geometry/Detector.hpp"
#include "core/messenger/exceptions.h"

// TODO [doc] This should partly move to a source file

namespace allpix {
    /**
     * @ingroup Delegates
     * @brief Container of the different delegate types
     *
     * A properly implemented delegate should only touch one of these fields.
     */
    struct DelegateTypes { // NOLINT
        std::shared_ptr<BaseMessage> single;
        std::vector<std::shared_ptr<BaseMessage>> multi;
        std::vector<std::pair<std::shared_ptr<BaseMessage>, std::string>> filter_multi;
    };
    /**
     * @ingroup Delegates
     * @brief Flags to change the behaviour of delegates
     *
     * All flags are distinct and can be combined using the | (OR) operator. The flags should be passed to the
     * \ref Messenger when \ref Messenger::registerFilter "registering" a filter or when binding either a \ref
     * Messenger::bindSingle "single" or \ref Messenger::bindMulti "multiple" messages. It depends on the delegate which
     * combination of flags is valid.
     */
    // TODO [doc] Is DelegateFlags or MessengerFlags a better name (and in separate file?)
    enum class MsgFlags : uint32_t {
        NONE = 0,                   ///< No enabled flags
        REQUIRED = (1 << 0),        ///< Require a message before running a module
        ALLOW_OVERWRITE = (1 << 1), ///< Allow overwriting a previous message
        IGNORE_NAME = (1 << 2),     ///< Listen to all ignoring message name (equal to * as a input configuration parameter)
        UNNAMED_ONLY = (1 << 3)     ///< Listen to all messages without explicit name (equal to ? as configuration parameter)
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
        explicit BaseDelegate(MsgFlags flags) : flags_(flags) {}
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
        BaseDelegate(BaseDelegate&&) = default;
        BaseDelegate& operator=(BaseDelegate&&) = default;
        /// @}

        /**
         * @brief Check if delegate has a required message
         * @return True if message is required, false otherwise
         */
        bool isRequired() const { return (getFlags() & MsgFlags::REQUIRED) != MsgFlags::NONE; }

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
         * @brief Process a message and forward it to its final destination
         * @param msg Message to process
         * @param name Name of the message
         * @param dest Destination of the message
         */
        virtual void process(std::shared_ptr<BaseMessage> msg, std::string name, DelegateTypes& dest) = 0;

    protected:
        MsgFlags flags_;
    };

    /**
     * @ingroup Delegates
     * @brief Base for all delegates operating on modules
     *
     * As all delegates currently operate on modules, this class is in fact the
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
        T* const obj_;
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
        void process(std::shared_ptr<BaseMessage> msg, std::string, DelegateTypes&) override {
            // Store the message and mark as processed
            messages_.push_back(msg);
        }

    private:
        std::vector<std::shared_ptr<BaseMessage>> messages_;
    };

    /**
     * @ingroup Delegates
     * @brief Delegate for filtering messages using a function
     */
    template <typename T, typename R> class FilterDelegate : public ModuleDelegate<T> {
    public:
        using FilterFunction = bool (T::*)(const std::shared_ptr<R>&) const;

        /**
         * @brief Construct a function delegate for the given module
         * @param flags Messenger flags
         * @param obj Module object this delegate should operate on
         * @param filter A filter function receiving a shared_ptr to the message and its name, returning a boolean indicating
         * the filter decision
         */
        FilterDelegate(MsgFlags flags, T* obj, FilterFunction filter) : ModuleDelegate<T>(flags, obj), filter_(filter) {}

        /**
         * @brief Calls the filter function with the supplied message
         * @param msg Message to process
         * @param dest Destination of the message
         * @warning The filter function is called directly from the delegate, no heavy processing should be done in the
         * filter function
         */
        void process(std::shared_ptr<BaseMessage> msg, std::string, DelegateTypes& dest) override {
#ifndef NDEBUG
            // The type names should have been correctly resolved earlier
            const BaseMessage* inst = msg.get();
            assert(typeid(*inst) == typeid(R));
#endif
            // Filter the message, and store it if it should be kept
            if((this->obj_->*filter_)(std::static_pointer_cast<R>(msg))) {
                dest.filter_multi.emplace_back(std::static_pointer_cast<R>(msg), "");
            }
        }

    private:
        FilterFunction filter_;
    };

    /**
     * @ingroup Delegates
     * @brief Delegate for invoking a filter listening to all messages also getting the name
     */
    template <typename T> class FilterAllDelegate : public ModuleDelegate<T> {
    public:
        using FilterFunction = bool (T::*)(const std::shared_ptr<BaseMessage>&, const std::string&) const;

        /**
         * @brief Construct a filter delegate for the given module
         * @param flags Messenger flags
         * @param obj Module object this delegate should operate on
         * @param filter A filter function receiving a shared_ptr to the message and its name, returning a boolean indicating
         * the filter decision
         */
        FilterAllDelegate(MsgFlags flags, T* obj, FilterFunction filter) : ModuleDelegate<T>(flags, obj), filter_(filter) {}

        /**
         * @brief Calls the filter function with the supplied message
         * @param msg Message to process
         * @param name Name of the message
         * @param dest Destination of the message
         * @warning The filter function is called directly from the delegate, no heavy processing should be done in the
         * filter function
         */
        void process(std::shared_ptr<BaseMessage> msg, std::string name, DelegateTypes& dest) override {
            // Filter the message, and store it if it should be kept
            if((this->obj_->*filter_)(std::static_pointer_cast<BaseMessage>(msg), name)) {
                dest.filter_multi.emplace_back(std::static_pointer_cast<BaseMessage>(msg), name);
            }
        }

    private:
        FilterFunction filter_;
    };

    /**
     * @ingroup Delegates
     * @brief Delegate for binding a single message
     */
    template <typename T, typename R> class SingleBindDelegate : public ModuleDelegate<T> {
    public:
        /**
         * @brief Construct a single bound delegate for the given module
         * @param flags Messenger flags
         * @param obj Module object this delegate should operate on
         */
        SingleBindDelegate(MsgFlags flags, T* obj) : ModuleDelegate<T>(flags, obj) {}

        /**
         * @brief Saves the message in the passed destination
         * @param msg Message to process
         * @param dest Destination of the message
         * @throws UnexpectedMessageException If this delegate has already received the message after the previous reset (not
         * thrown if the \ref MsgFlags::ALLOW_OVERWRITE "ALLOW_OVERWRITE" flag is passed)
         *
         * The saved value is overwritten if the \ref MsgFlags::ALLOW_OVERWRITE "ALLOW_OVERWRITE" flag is enabled.
         */
        void process(std::shared_ptr<BaseMessage> msg, std::string, DelegateTypes& dest) override {
#ifndef NDEBUG
            // The type names should have been correctly resolved earlier
            const BaseMessage* inst = msg.get();
            assert(typeid(*inst) == typeid(R));
#endif
            // Raise an error if the message is overwritten (unless it is allowed)
            if(dest.single != nullptr && (this->getFlags() & MsgFlags::ALLOW_OVERWRITE) == MsgFlags::NONE) {
                throw UnexpectedMessageException(this->obj_->getUniqueName(), typeid(R));
            }

            // Save the message
            dest.single = std::static_pointer_cast<R>(msg);
        }
    };

    /**
     * @ingroup Delegates
     * @brief Delegate for binding multiple message to a vector
     */
    template <typename T, typename R> class VectorBindDelegate : public ModuleDelegate<T> {
    public:
        /**
         * @brief Construct a vector bound delegate for the given module
         * @param flags Messenger flags
         * @param obj Module object this delegate should operate on
         */
        VectorBindDelegate(MsgFlags flags, T* obj) : ModuleDelegate<T>(flags, obj) {}

        /**
         * @brief Adds the message to the bound vector
         * @param msg Message to process
         * @param dest Destination of the message
         */
        void process(std::shared_ptr<BaseMessage> msg, std::string, DelegateTypes& dest) override {
#ifndef NDEBUG
            // The type names should have been correctly resolved earlier
            const BaseMessage* inst = msg.get();
            assert(typeid(*inst) == typeid(R));
#endif
            // Add the message to the vector
            dest.multi.push_back(std::static_pointer_cast<R>(msg));
        }
    };
} // namespace allpix

#endif /* ALLPIX_DELEGATE_H */
