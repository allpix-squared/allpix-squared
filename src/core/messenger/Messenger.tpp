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

    template <typename T, typename R, MsgFlags flags = MsgFlags::NONE> void Messenger::bindSingle(T* receiver) {
        static_assert(std::is_base_of<Module, T>::value, "Receiver should have Module as a base class");
        static_assert(std::is_base_of<BaseMessage, R>::value,
                      "Bound variable should be a shared pointer to a message derived from the Message class");

        auto delegate = std::make_shared<SingleBindDelegate<T, R>>(flags, receiver);
        add_delegate(typeid(R), receiver, std::move(delegate));
    }

    template <typename T, typename R, MsgFlags flags = MsgFlags::NONE> void Messenger::bindMulti(T* receiver) {
        static_assert(std::is_base_of<Module, T>::value, "Receiver should have Module as a base class");
        static_assert(std::is_base_of<BaseMessage, R>::value,
                      "Bound variable should be a shared pointer to a message derived from the Message class");

        auto delegate = std::make_shared<VectorBindDelegate<T, R>>(flags, receiver);
        add_delegate(typeid(R), receiver, std::move(delegate));
    }

} // namespace allpix
