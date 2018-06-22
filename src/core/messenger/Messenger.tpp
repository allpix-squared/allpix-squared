namespace allpix {
    template <typename T>
    void
    Messenger::dispatchMessage(unsigned int event_id, Module* source, std::shared_ptr<T> message, const std::string& name) {
        static_assert(std::is_base_of<BaseMessage, T>::value, "Dispatched message should inherit from Message class");

        // Tag the message
        auto base_message = std::static_pointer_cast<BaseMessage>(message);
        base_message->event_id = event_id;

        dispatch_message(source, base_message, name);
    }

    template <typename T>
    void Messenger::registerListener(T* receiver,
                                     void (T::*method)(std::shared_ptr<BaseMessage>, std::string name),
                                     MsgFlags flags) {
        static_assert(std::is_base_of<Module, T>::value, "Receiver should have Module as a base class");
        auto delegate = std::make_shared<FunctionAllDelegate<T>>(flags, receiver, method);
        add_delegate(typeid(BaseMessage), receiver, std::move(delegate), typeid(T));
    }

    template <typename T, typename R>
    void Messenger::registerListener(T* receiver, void (T::*method)(std::shared_ptr<R>), MsgFlags flags) {
        static_assert(std::is_base_of<Module, T>::value, "Receiver should have Module as a base class");
        static_assert(
            std::is_base_of<BaseMessage, R>::value,
            "Notifier method should take a shared pointer to a message derived from the Message class as argument");

        auto delegate = std::make_shared<FunctionDelegate<T, R>>(flags, receiver, method);
        add_delegate(typeid(R), receiver, std::move(delegate), typeid(T));
    }

    template <typename T, typename R> void Messenger::bindSingle(T* receiver, MessageStorage<R> T::*member, MsgFlags flags) {
        static_assert(std::is_base_of<Module, T>::value, "Receiver should have Module as a base class");
        // TODO: update error message
        static_assert(std::is_base_of<BaseMessage, R>::value,
                      "Bound variable should be a shared pointer to a message derived from the Message class");

        auto delegate = std::make_shared<SingleBindDelegate<T, R>>(flags, receiver, member);
        add_delegate(typeid(R), receiver, std::move(delegate), typeid(T));
    }

    // FIXME: Allow binding other containers besides vector
    template <typename T, typename R>
    void Messenger::bindMulti(T* receiver, std::vector<std::shared_ptr<R>> T::*member, MsgFlags flags) {
        static_assert(std::is_base_of<Module, T>::value, "Receiver should have Module as a base class");
        static_assert(std::is_base_of<BaseMessage, R>::value,
                      "Bound variable should be a shared pointer to a message derived from the Message class");

        auto delegate = std::make_shared<VectorBindDelegate<T, R>>(flags, receiver, member);
        add_delegate(typeid(R), receiver, std::move(delegate), typeid(T));
    }
} // namespace allpix
