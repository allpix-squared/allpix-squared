namespace allpix {
    // TODO [doc] This method should be removed
    template <typename T> void Messenger::dispatchMessage(Module *source, const T& message, const std::string& name) {
        static_assert(std::is_base_of<BaseMessage, T>::value, "Dispatched message should inherit from Message class");
        std::shared_ptr<BaseMessage> msg_ptr = std::make_shared<T>(message);
        dispatch_message(source, msg_ptr, name);
    }
    template <typename T> void Messenger::dispatchMessage(Module *source, std::shared_ptr<T> message, const std::string& name) {
        static_assert(std::is_base_of<BaseMessage, T>::value, "Dispatched message should inherit from Message class");
        dispatch_message(source, std::static_pointer_cast<BaseMessage>(message), name);
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
        add_delegate(typeid(R), message_name, receiver, std::move(delegate));
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
        add_delegate(typeid(R), message_name, receiver, std::move(delegate));
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
        add_delegate(typeid(R), message_name, receiver, std::move(delegate));
    }
}
