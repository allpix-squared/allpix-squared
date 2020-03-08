/**
 * @file
 * @brief Template implementation of messenger
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

namespace allpix {
    template <typename T>
    void Messenger::dispatchMessage(Module* source, std::shared_ptr<T> message, const std::string& name) {
        static_assert(std::is_base_of<BaseMessage, T>::value, "Dispatched message should inherit from Message class");
        dispatch_message(source, std::static_pointer_cast<BaseMessage>(message), name);
    }

    template <typename T>
    void Messenger::registerListener(T* receiver,
                                     void (T::*method)(std::shared_ptr<BaseMessage>, std::string name),
                                     MsgFlags flags) {
        static_assert(std::is_base_of<Module, T>::value, "Receiver should have Module as a base class");
        auto delegate = std::make_unique<FunctionAllDelegate<T>>(flags, receiver, method);
        add_delegate(typeid(BaseMessage), receiver, std::move(delegate));
    }

    template <typename T, typename R>
    void Messenger::registerListener(T* receiver, void (T::*method)(std::shared_ptr<R>), MsgFlags flags) {
        static_assert(std::is_base_of<Module, T>::value, "Receiver should have Module as a base class");
        static_assert(
            std::is_base_of<BaseMessage, R>::value,
            "Notifier method should take a shared pointer to a message derived from the Message class as argument");

        auto delegate = std::make_unique<FunctionDelegate<T, R>>(flags, receiver, method);
        add_delegate(typeid(R), receiver, std::move(delegate));
    }

    template <typename T, typename R>
    void Messenger::bindSingle(T* receiver, std::shared_ptr<R> T::*member, MsgFlags flags) {
        static_assert(std::is_base_of<Module, T>::value, "Receiver should have Module as a base class");
        static_assert(std::is_base_of<BaseMessage, R>::value,
                      "Bound variable should be a shared pointer to a message derived from the Message class");

        auto delegate = std::make_unique<SingleBindDelegate<T, R>>(flags, receiver, member);
        add_delegate(typeid(R), receiver, std::move(delegate));
    }

    // FIXME: Allow binding other containers besides vector
    template <typename T, typename R>
    void Messenger::bindMulti(T* receiver, std::vector<std::shared_ptr<R>> T::*member, MsgFlags flags) {
        static_assert(std::is_base_of<Module, T>::value, "Receiver should have Module as a base class");
        static_assert(std::is_base_of<BaseMessage, R>::value,
                      "Bound variable should be a shared pointer to a message derived from the Message class");

        auto delegate = std::make_unique<VectorBindDelegate<T, R>>(flags, receiver, member);
        add_delegate(typeid(R), receiver, std::move(delegate));
    }
} // namespace allpix
