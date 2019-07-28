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

    template <typename T, typename R, MsgFlags flags> void Messenger::bindSingle(T* receiver) {
        static_assert(std::is_base_of<Module, T>::value, "Receiver should have Module as a base class");
        static_assert(std::is_base_of<BaseMessage, R>::value,
                      "Bound variable should be a shared pointer to a message derived from the Message class");

        auto delegate = std::make_shared<SingleBindDelegate<T, R>>(flags, receiver);
        add_delegate(typeid(R), receiver, std::move(delegate));
    }

    template <typename T, typename R, MsgFlags flags> void Messenger::bindMulti(T* receiver) {
        static_assert(std::is_base_of<Module, T>::value, "Receiver should have Module as a base class");
        static_assert(std::is_base_of<BaseMessage, R>::value,
                      "Bound variable should be a shared pointer to a message derived from the Message class");

        auto delegate = std::make_shared<VectorBindDelegate<T, R>>(flags, receiver);
        add_delegate(typeid(R), receiver, std::move(delegate));
    }

    template <typename T>
    void Messenger::dispatchMessage(Module* module, std::shared_ptr<T> message, const std::string& name) {
        local_messenger_->dispatchMessage(module, message, name);
    }

    template <typename T> std::shared_ptr<T> Messenger::fetchMessage(Module* module) {
        try {
            return local_messenger_->fetchMessage<T>(module);
        } catch(const std::out_of_range& e) {
            throw MessageNotFoundException(module->getUniqueName(), typeid(T));
        }
    }

    template <typename T> std::vector<std::shared_ptr<T>> Messenger::fetchMultiMessage(Module* module) {
        try {
            return local_messenger_->fetchMultiMessage<T>(module);
        } catch(const std::out_of_range& e) {
            throw MessageNotFoundException(module->getUniqueName(), typeid(T));
        }
    }

    template <typename T> std::shared_ptr<T> Messenger::LocalMessenger::fetchMessage(Module* module) {
        static_assert(std::is_base_of<BaseMessage, T>::value, "Fetched message should inherit from Message class");
        std::type_index type_idx = typeid(T);
        return std::static_pointer_cast<T>(messages_.at(module->getUniqueName()).at(type_idx).single);
    }

    template <typename T> std::vector<std::shared_ptr<T>> Messenger::LocalMessenger::fetchMultiMessage(Module* module) {
        static_assert(std::is_base_of<BaseMessage, T>::value, "Fetched message should inherit from Message class");

        // TODO: do nothing if T == BaseMessage; there is no need to cast (optimized out)?
        std::type_index type_idx = typeid(T);

        // Construct an empty vector in case no previous modules created one during dispatch
        auto base_messages = messages_.at(module->getUniqueName()).at(type_idx).multi;

        std::vector<std::shared_ptr<T>> derived_messages;
        for(auto& message : base_messages) {
            derived_messages.push_back(std::static_pointer_cast<T>(message));
        }

        return derived_messages;
    }

} // namespace allpix
