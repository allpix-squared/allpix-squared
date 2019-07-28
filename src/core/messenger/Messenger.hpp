/**
 * @file
 * @brief Send objects between modules using a messenger
 * @copyright Copyright (c) 2017-2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MESSENGER_H
#define ALLPIX_MESSENGER_H

#include <list>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>

#include "../module/Module.hpp"
#include "Message.hpp"
#include "delegates.h"

namespace allpix {

    /**
     * @ingroup Managers
     * @brief Manager responsible for setting up communication between objects and sending messages between them
     *
     * Registers and sets up communication (delegates) from modules to other listening modules. Dispatches messages from
     * modules to other listening modules. There are various way to receive the messages using \ref Delegates. Messages are
     * only send to modules listening to the exact same type of message.
     */
    class Messenger {
        friend class Module;

    public:
        /**
         * @brief Construct the messenger
         */
        Messenger();
        /**
         * @brief Default destructor (checks if delegates are removed in DEBUG)
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
         * @brief Register a function filtering all dispatched messages
         * @param receiver Receiving module
         * @param filter Filter function in the module (fetching a pointer to the base message and the name of the message)
         * @param flags Message configuration flags (defaults to \ref MsgFlags::IGNORE_NAME "ignoring the message name")
         */
        template <typename T>
        void registerFilter(T* receiver,
                            bool (T::*filter)(const std::shared_ptr<BaseMessage>&, const std::string& name) const,
                            MsgFlags flags = MsgFlags::IGNORE_NAME);

        /**
         * @brief Register a function filtering a particular message
         * @param receiver Receiving module
         * @param filter Filter function in the module (fetching a pointer to the message)
         * @param flags Message configuration flags
         */
        template <typename T, typename R>
        void
        registerFilter(T* receiver, bool (T::*filter)(const std::shared_ptr<R>&) const, MsgFlags flags = MsgFlags::NONE);

        /**
         * @brief Register subscription for a single message
         * @param receiver Receiving module
         * @param flags Message configuration flags
         * @warning This allows to only receive a single message of the type per run unless the
         *           \ref MsgFlags::ALLOW_OVERWRITE "ALLOW_OVERWRITE" flag is passed
         */
        template <typename T, typename R, MsgFlags flags = MsgFlags::NONE> void bindSingle(T* receiver);

        /**
         * @brief Register subscription for multiple messages
         * @param receiver Receiving module
         * @param member Pointer to the vector of messages to listen to
         * @param flags Message configuration flags
         */
        template <typename T, typename R, MsgFlags flags = MsgFlags::NONE> void bindMulti(T* receiver);

        /**
         * @brief Dispatches a message to subscribing modules
         * @param message Pointer to the message to dispatch
         * @param name Optional message name (defaults to - indicating that it should dispatch to the module output
         * parameter)
         */
        template <typename T> void dispatchMessage(Module* module, std::shared_ptr<T> message, const std::string&);

        /**
         * @brief Fetches a single message of specified type meant for the calling module
         * @return Shared pointer to message
         */
        template <typename T> std::shared_ptr<T> fetchMessage(Module* module);

        /**
         * @brief Fetches multiple messages of specified type meant for the calling module
         * @return Vector of shared pointers to messages
         */
        template <typename T> std::vector<std::shared_ptr<T>> fetchMultiMessage(Module* module);

        /**
         * @brief Fetches filtered messages meant for the calling module
         * @return Vector of pairs containing shared pointer to and name of message
         */
        std::vector<std::pair<std::shared_ptr<BaseMessage>, std::string>> fetchFilteredMessages(Module* module);

        /**
         * @brief Check if a specific message has a receiver
         * @param source Module that will send the message
         * @param message Instantiation of the message to check
         * @return True if the message has at least one receiver, false otherwise
         */
        bool hasReceiver(Module* source, const std::shared_ptr<BaseMessage>& message);

        /**
         * @brief Check if a delegate has recieved its message
         * @return True if satisfied, false otherwise
         */
        bool isSatisfied(BaseDelegate* delegate) const;

        /**
         * @brief Resets the messenger and clear any stored messages.
         */
        void reset();

    private:
        /**
         * @brief Add a delegate to the listeners
         * @param message_type Type the delegate listens to
         * @param module Module linked to the delegate
         * @param delegate Delegate that listens to the message
         */
        void add_delegate(const std::type_info& message_type, Module* module, const std::shared_ptr<BaseDelegate>& delegate);

        /**
         * @brief Removes a delegate from the listeners
         * @param delegate The delegate to remove
         * @note This should be called by the Module destructor to remove their delegates
         */
        void remove_delegate(BaseDelegate* delegate);

        using DelegateMap = std::map<std::type_index, std::map<std::string, std::list<std::shared_ptr<BaseDelegate>>>>;
        using DelegateIteratorMap =
            std::map<BaseDelegate*,
                     std::tuple<std::type_index, std::string, std::list<std::shared_ptr<BaseDelegate>>::iterator>>;

        DelegateMap delegates_;
        DelegateIteratorMap delegate_to_iterator_;

        /**
         * @breif Responsible for the actual handling of messages between Modules.
         *
         * The local messenger is an internal object that is allocated for each thread seperatly. It handles dispatching
         * and fetching messages between Modules.
         */
        class LocalMessenger {
        public:
            LocalMessenger(Messenger& global_messenger);

            /**
             * @brief Resets the messenger and clear any stored messages.
             */
            void reset();

            void dispatch_message(Module* source, std::shared_ptr<BaseMessage> message, std::string name);
            bool dispatch_message(Module* source,
                                  const std::shared_ptr<BaseMessage>& message,
                                  const std::string& name,
                                  const std::string& id);

            /**
             * @brief Check if a delegate has recieved its message
             * @return True if satisfied, false otherwise
             */
            bool isSatisfied(BaseDelegate* delegate) const;

            /**
             * @brief Fetches a single message of specified type meant for the calling module
             * @return Shared pointer to message
             */
            template <typename T> std::shared_ptr<T> fetchMessage(Module* module);

            /**
             * @brief Fetches multiple messages of specified type meant for the calling module
             * @return Vector of shared pointers to messages
             */
            template <typename T> std::vector<std::shared_ptr<T>> fetchMultiMessage(Module* module);

            /**
             * @brief Fetches filtered messages meant for the calling module
             * @return Vector of pairs containing shared pointer to and name of message
             */
            std::vector<std::pair<std::shared_ptr<BaseMessage>, std::string>> fetchFilteredMessages(Module* module);

        private:
            Messenger& global_messenger_;

            std::unordered_map<std::string, std::unordered_map<std::type_index, DelegateTypes>> messages_;
            std::vector<std::shared_ptr<BaseMessage>> sent_messages_;
        };

        static thread_local std::unique_ptr<LocalMessenger> local_messenger_;

        mutable std::mutex mutex_;
    };
} // namespace allpix

// Include template members
#include "Messenger.tpp"

#endif /* ALLPIX_MESSENGER_H */
