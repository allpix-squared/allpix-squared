/**
 * @file
 * @brief Template implementation of messenger
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

namespace allpix {
    template <typename T>
    void Messenger::registerFilter(T* receiver,
                                   bool (T::*method)(const std::shared_ptr<BaseMessage>&, const std::string& name) const,
                                   MsgFlags flags) {
        static_assert(std::is_base_of<Module, T>::value, "Receiver should have Module as a base class");
        auto delegate = std::make_shared<FilterAllDelegate<T>>(flags, receiver, method);
        add_delegate(typeid(BaseMessage), receiver, std::move(delegate));
    }

    template <typename T, typename R>
    void Messenger::registerFilter(T* receiver, bool (T::*method)(const std::shared_ptr<R>&) const, MsgFlags flags) {
        static_assert(std::is_base_of<Module, T>::value, "Receiver should have Module as a base class");
        static_assert(
            std::is_base_of<BaseMessage, R>::value,
            "Notifier method should take a shared pointer to a message derived from the Message class as argument");

        auto delegate = std::make_shared<FilterDelegate<T, R>>(flags, receiver, method);
        add_delegate(typeid(R), receiver, std::move(delegate));
    }

    template <typename T> void Messenger::bindSingle(Module* receiver, MsgFlags flags) {
        static_assert(std::is_base_of<BaseMessage, T>::value,
                      "Bound variable should be a shared pointer to a message derived from the Message class");

        auto delegate = std::make_shared<SingleBindDelegate<Module, T>>(flags, receiver);
        add_delegate(typeid(T), receiver, std::move(delegate));
    }

    template <typename T> void Messenger::bindMulti(Module* receiver, MsgFlags flags) {
        static_assert(std::is_base_of<BaseMessage, T>::value,
                      "Bound variable should be a shared pointer to a message derived from the Message class");

        auto delegate = std::make_shared<VectorBindDelegate<Module, T>>(flags, receiver);
        add_delegate(typeid(T), receiver, std::move(delegate));
    }

    template <typename T>
    void Messenger::dispatchMessage(Module* module, std::shared_ptr<T> message, Event* event, const std::string& name) {
        auto* local_messenger = event->get_local_messenger();
        local_messenger->dispatchMessage(module, std::move(message), name);
    }

    template <typename T> std::shared_ptr<T> Messenger::fetchMessage(Module* module, Event* event) {
        try {
            auto* local_messenger = event->get_local_messenger();
            return local_messenger->fetchMessage<T>(module);
        } catch(const std::out_of_range& e) {
            throw MessageNotFoundException(module->getUniqueName(), typeid(T));
        }
    }

    template <typename T> std::vector<std::shared_ptr<T>> Messenger::fetchMultiMessage(Module* module, Event* event) {
        try {
            auto* local_messenger = event->get_local_messenger();
            return local_messenger->fetchMultiMessage<T>(module);
        } catch(const std::out_of_range& e) {
            throw MessageNotFoundException(module->getUniqueName(), typeid(T));
        }
    }

    template <typename T> std::shared_ptr<T> LocalMessenger::fetchMessage(Module* module) {
        static_assert(std::is_base_of<BaseMessage, T>::value, "Fetched message should inherit from Message class");
        std::type_index type_idx = typeid(T);
        return std::static_pointer_cast<T>(messages_.at(module->getUniqueName()).at(type_idx).single);
    }

    template <typename T> std::vector<std::shared_ptr<T>> LocalMessenger::fetchMultiMessage(Module* module) {
        static_assert(std::is_base_of<BaseMessage, T>::value, "Fetched message should inherit from Message class");

        // TODO: do nothing if T == BaseMessage; there is no need to cast (optimized out)?
        std::type_index type_idx = typeid(T);

        // Construct an empty vector in case no previous modules created one during dispatch
        auto base_messages = messages_.at(module->getUniqueName()).at(type_idx).multi;

        std::vector<std::shared_ptr<T>> derived_messages;
        derived_messages.reserve(base_messages.size());
        for(auto& message : base_messages) {
            derived_messages.push_back(std::static_pointer_cast<T>(message));
        }

        return derived_messages;
    }

} // namespace allpix
