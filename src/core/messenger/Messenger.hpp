/**
 * @file
 * @brief Send objects between modules using a messenger
 * @copyright MIT License
 */

#ifndef ALLPIX_MESSENGER_H
#define ALLPIX_MESSENGER_H

#include <map>
#include <memory>
#include <typeindex>
#include <utility>
#include <vector>

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
    public:
        /**
         * @brief Construct the messenger
         */
        Messenger();

        /// @{
        /**
         * @brief Copying the messenger is not allowed
         */
        Messenger(const Messenger&) = delete;
        Messenger& operator=(const Messenger&) = delete;
        /// @}

        // FIXME: unregistering of listeners still need to be added (as well as checking for proper deletion)...

        // TODO [doc] Fetch the message type from the configuration instead

        // Register a listener
        // FIXME: is the empty string a proper catch-all or do we want to have a clearer separation?

        /// @{
        /**
         * @brief Register a function listening for a particular message
         * @param receiver Receiving module
         * @param method Listener function in the module (fetching a pointer to the message)
         * @param flags Message configuration flags
         */
        template <typename T, typename R>
        void registerListener(T* receiver, void (T::*method)(std::shared_ptr<R>), MsgFlags flags);
        /**
         * @brief Register a function listening for a particular message
         * @param receiver Receiving module
         * @param method Listener function in the module (fetching a pointer to the specific message)
         * @param flags Message configuration flags (defaults to none)
         * @param message_name Name of the message to listen to (defaults to all messages)
         */
        // TODO [doc] Remove this method (as message name is determined by messenger)
        template <typename T, typename R>
        void registerListener(T* receiver,
                              void (T::*)(std::shared_ptr<R>),
                              const std::string& message_name = "",
                              MsgFlags flags = MsgFlags::NONE);
        /// @}

        /// @{
        /**
         * @brief Binds a pointer to a single message
         * @param receiver Receiving module
         * @param member Pointer to the message to listen to
         * @param flags Message configuration flags
         * @warning This allows to only receive a single message of the type per run unless the
         *           \ref MsgFlags::ALLOW_OVERWRITE "ALLOW_OVERWRITE" flag is passed
         */
        template <typename T, typename R> void bindSingle(T* receiver, std::shared_ptr<R> T::*, MsgFlags flags);
        /**
         * @brief Binds a pointer to a single message
         * @param receiver Receiving module
         * @param member Pointer to the message to listen to
         * @param flags Message configuration flags (defaults to none)
         * @param message_name Name of the message to listen to (defaults to all messages)
         * @warning This allows to only receive a single message of the type per run unless the
         *           \ref MsgFlags::ALLOW_OVERWRITE "ALLOW_OVERWRITE" flag is passed
         */
        // TODO [doc] Remove this method (as message name is determined by messenger)
        template <typename T, typename R>
        void bindSingle(T* receiver,
                        std::shared_ptr<R> T::*,
                        const std::string& message_name = "",
                        MsgFlags flags = MsgFlags::NONE);
        /// @}

        /// @{
        /**
         * @brief Binds a pointer to a list of messages
         * @param receiver Receiving module
         * @param member Pointer to the vector of messages to listen to
         * @param flags Message configuration flags
         */
        // TODO [doc] Better name?
        template <typename T, typename R> void bindMulti(T* receiver, std::vector<std::shared_ptr<R>> T::*, MsgFlags flags);
        /**
         * @brief Binds a pointer to a list of messages
         * @param receiver Receiving module
         * @param member Pointer to the vector of messages to listen to
         * @param flags Message configuration flags (defaults to none)
         * @param message_name Name of the message to listen to (defaults to all messages)
         */
        // TODO [doc] Remove this method (as message name is determined by messenger)
        template <typename T, typename R>
        void bindMulti(T* receiver,
                       std::vector<std::shared_ptr<R>> T::*,
                       const std::string& message_name = "",
                       MsgFlags flags = MsgFlags::NONE);
        /// @}

        /// @{
        /**
         * @brief Dispatches a message
         * @param msg Message to dispatch
         * @param name Name of the message
         * @warning This method should not be used as it does an internal copy to a shared_ptr (which should be used
         * directly)
         */
        // TODO [doc] Remove this method
        template <typename T> void dispatchMessage(const T& msg, const std::string& name = "");
        /**
         * @brief Dispatches a message
         * @param msg Pointer to the message to dispatch
         * @param name Name of the message
         */
        // TODO [doc] The source module should also be required as parameter
        template <typename T> void dispatchMessage(std::shared_ptr<T> msg, const std::string& name = "");
        /// @}

    private:
        /**
         * @brief Add a delegate to the list of listeners
         * @param message_type Type the delegate listens to
         * @param message_name Name of the message the delegate listens to
         * @param delegate Delegate that listens to the message
         */
        void add_delegate(const std::type_info& message_type,
                          const std::string& message_name,
                          std::unique_ptr<BaseDelegate> delegate);

        /**
         * @brief Dispatch base message to the correct delegates
         * @param msg Message to dispatch
         * @param name Name of the message
         */
        void dispatch_message(const std::shared_ptr<BaseMessage>& msg, const std::string& name = "");

        using DelegateMap = std::map<std::type_index, std::map<std::string, std::vector<std::unique_ptr<BaseDelegate>>>>;

        DelegateMap delegates_;
    };

    // TODO [doc] This should be copied to an source implementation file

    // WARNING: Provide this option or not (leads to copying)
    template <typename T> void Messenger::dispatchMessage(const T& msg, const std::string& name) {
        static_assert(std::is_base_of<BaseMessage, T>::value, "Dispatched message should inherit from Message class");
        std::shared_ptr<BaseMessage> msg_ptr = std::make_shared<T>(msg);
        dispatch_message(msg_ptr, name);
    }
    template <typename T> void Messenger::dispatchMessage(std::shared_ptr<T> msg, const std::string& name) {
        static_assert(std::is_base_of<BaseMessage, T>::value, "Dispatched message should inherit from Message class");
        dispatch_message(std::static_pointer_cast<BaseMessage>(msg), name);
    }

    template <typename T, typename R>
    void Messenger::registerListener(T* receiver, void (T::*method)(std::shared_ptr<R>), MsgFlags flags) {
        registerListener(receiver, method, "", flags);
    }
    template <typename T, typename R>
    void Messenger::registerListener(T* receiver,
                                     void (T::*method)(std::shared_ptr<R>),
                                     const std::string& message_name,
                                     MsgFlags flags) {
        static_assert(std::is_base_of<Module, T>::value, "Receiver should have Module as a base class");
        static_assert(
            std::is_base_of<BaseMessage, R>::value,
            "Notifier method should take a shared pointer to a message derived from the Message class as argument");

        auto delegate = std::make_unique<FunctionDelegate<T, R>>(flags, receiver, method);
        receiver->add_delegate(delegate.get());
        add_delegate(typeid(R), message_name, std::move(delegate));
    }

    template <typename T, typename R>
    void Messenger::bindSingle(T* receiver, std::shared_ptr<R> T::*member, MsgFlags flags) {
        bindSingle(receiver, member, "", flags);
    }
    template <typename T, typename R>
    void Messenger::bindSingle(T* receiver, std::shared_ptr<R> T::*member, const std::string& message_name, MsgFlags flags) {
        static_assert(std::is_base_of<Module, T>::value, "Receiver should have Module as a base class");
        static_assert(std::is_base_of<BaseMessage, R>::value,
                      "Bound variable should be a shared pointer to a message derived from the Message class");

        auto delegate = std::make_unique<SingleBindDelegate<T, R>>(flags, receiver, member);
        receiver->add_delegate(delegate.get());
        add_delegate(typeid(R), message_name, std::move(delegate));
    }

    // FIXME: Allow binding other containers besides vector
    template <typename T, typename R>
    void Messenger::bindMulti(T* receiver, std::vector<std::shared_ptr<R>> T::*member, MsgFlags flags) {
        bindMulti(receiver, member, "", flags);
    }
    template <typename T, typename R>
    void Messenger::bindMulti(T* receiver,
                              std::vector<std::shared_ptr<R>> T::*member,
                              const std::string& message_name,
                              MsgFlags flags) {
        static_assert(std::is_base_of<Module, T>::value, "Receiver should have Module as a base class");
        static_assert(std::is_base_of<BaseMessage, R>::value,
                      "Bound variable should be a shared pointer to a message derived from the Message class");

        auto delegate = std::make_unique<VectorBindDelegate<T, R>>(flags, receiver, member);
        receiver->add_delegate(delegate.get());
        add_delegate(typeid(R), message_name, std::move(delegate));
    }
} // namespace allpix

#endif /* ALLPIX_MESSENGER_H */
