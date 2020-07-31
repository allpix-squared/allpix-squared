/**
 * @file
 * @brief Send objects between modules using a messenger
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MESSENGER_H
#define ALLPIX_MESSENGER_H

#include <list>
#include <map>
#include <memory>
#include <typeindex>
#include <utility>

#include "Message.hpp"
#include "core/module/Module.hpp"
#include "delegates.h"

namespace allpix {

    /**
     * @ingroup Managers
     * @brief Manager responsible for sending messages between objects
     *
     * Dispatches messages from modules to other listening modules. There are various way to receive the messages using
     * \ref Delegates. Messages are only send to modules listening to the exact same type of message.
     */
    class Messenger {
        friend class Module;

    public:
        /**
         * @brief Construct the messenger
         */
        Messenger();
        /**
         * @brief Default destructor (checks if delegates are removed)
         */
        ~Messenger();

        /// @{
        /**
         * @brief Copying the messenger is not allowed
         */
        Messenger(const Messenger&) = delete;
        Messenger& operator=(const Messenger&) = delete;
        /// @}

        /// @{
        /**
         * @brief Disallow move because of mutex
         */
        Messenger(Messenger&&) = delete;
        Messenger& operator=(Messenger&&) = delete;
        /// @}

        /**
         * @brief Register a function listening to all dispatched messages
         * @param receiver Receiving module
         * @param method Listener function in the module (fetching a pointer to the base message and the name of the message)
         * @param flags Message configuration flags (defaults to \ref MsgFlags::IGNORE_NAME "ignoring the message name")
         */
        template <typename T>
        void registerListener(T* receiver,
                              void (T::*method)(std::shared_ptr<BaseMessage>, std::string name),
                              MsgFlags flags = MsgFlags::IGNORE_NAME);

        /**
         * @brief Register a function listening for a particular message
         * @param receiver Receiving module
         * @param method Listener function in the module (fetching a pointer to the message)
         * @param flags Message configuration flags
         */
        template <typename T, typename R>
        void registerListener(T* receiver, void (T::*method)(std::shared_ptr<R>), MsgFlags flags = MsgFlags::NONE);

        /**
         * @brief Binds a pointer to a single message
         * @param receiver Receiving module
         * @param member Pointer to the message to listen to
         * @param flags Message configuration flags
         * @warning This allows to only receive a single message of the type per run unless the
         *           \ref MsgFlags::ALLOW_OVERWRITE "ALLOW_OVERWRITE" flag is passed
         */
        template <typename T, typename R>
        void bindSingle(T* receiver, std::shared_ptr<R> T::*member, MsgFlags flags = MsgFlags::NONE);

        /**
         * @brief Binds a pointer to a list of messages
         * @param receiver Receiving module
         * @param member Pointer to the vector of messages to listen to
         * @param flags Message configuration flags
         */
        // TODO [doc] Better name?
        template <typename T, typename R>
        void bindMulti(T* receiver, std::vector<std::shared_ptr<R>> T::*member, MsgFlags flags = MsgFlags::NONE);

        /**
         * @brief Check if a specific message has a receiver
         * @param source Module that will send the message
         * @param message Instantiation of the message to check
         * @return True if the message has at least one receiver, false otherwise
         */
        bool hasReceiver(Module* source, const std::shared_ptr<BaseMessage>& message);

        /**
         * @brief Dispatches a message
         * @param source Module dispatching the message
         * @param message Pointer to the message to dispatch
         * @param name Optional message name (defaults to - indicating that it should dispatch to the module output
         * parameter)
         */
        template <typename T>
        void dispatchMessage(Module* source, std::shared_ptr<T> message, const std::string& name = "-");

        /**
         * @brief Removes the list of sent messages, clearing them from memory if not otherwise used
         */
        inline void clearMessages() { sent_messages_.clear(); }

    private:
        /**
         * @brief Add a delegate to the listeners
         * @param message_type Type the delegate listens to
         * @param module Module linked to the delegate
         * @param delegate Delegate that listens to the message
         */
        void add_delegate(const std::type_info& message_type, Module* module, std::unique_ptr<BaseDelegate> delegate);

        /**
         * @brief Removes a delegate from the listeners
         * @param delegate The delegate to remove
         * @note This should be called by the Module destructor to remove their delegates
         */
        void remove_delegate(BaseDelegate* delegate);

        /**
         * @brief Dispatch base message to the specific and general delegates
         * @param source Dispatching module
         * @param message Message to dispatch
         * @param name Message name (- indicates to use module output parameter)
         */
        void dispatch_message(Module* source, const std::shared_ptr<BaseMessage>& message, std::string name);

        /**
         * @brief Dispatch base message to the exact delegates
         * @param source Dispatching module
         * @param message Message to dispatch
         * @param name Name of the message
         * @param id Identifier to dispatch to (either the name or '*' to dispatch to all)
         */
        bool dispatch_message(Module* source,
                              const std::shared_ptr<BaseMessage>& message,
                              const std::string& name,
                              const std::string& id);

        using DelegateMap = std::map<std::type_index, std::map<std::string, std::list<std::unique_ptr<BaseDelegate>>>>;
        using DelegateIteratorMap =
            std::map<BaseDelegate*,
                     std::tuple<std::type_index, std::string, std::list<std::unique_ptr<BaseDelegate>>::iterator>>;

        DelegateMap delegates_;
        DelegateIteratorMap delegate_to_iterator_;
        std::vector<std::shared_ptr<BaseMessage>> sent_messages_;

        mutable std::mutex mutex_;
    };
} // namespace allpix

// Include template members
#include "Messenger.tpp"

#endif /* ALLPIX_MESSENGER_H */
