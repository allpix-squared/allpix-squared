namespace allpix {
    // TODO: merge MessageStorage into Event?
    template <typename T> void Event::dispatchMessage(std::shared_ptr<T> message, const std::string& name) {
        static_assert(std::is_base_of<BaseMessage, T>::value, "Dispatched message should inherit from Message class");
        message_storage_.dispatch_message(message_storage_.module_, message, name);
    }

    template <typename T> std::shared_ptr<T> Event::fetchMessage() {
        static_assert(std::is_base_of<BaseMessage, T>::value, "Fetched message should inherit from Message class");
        return std::static_pointer_cast<T>(message_storage_.messages_.at(message_storage_.module_->getUniqueName()).single);
    }

    template <typename T> std::vector<std::shared_ptr<T>> Event::fetchMultiMessage() {
        static_assert(std::is_base_of<BaseMessage, T>::value, "Fetched message should inherit from Message class");

        // Yes, use operator[] here to construct empty vector in case none exist already.
        // This mirrors the case when a listening multiMessage-module receieves no messages, so it's target vector would be
        // empty.
        auto base_messages = message_storage_.messages_[message_storage_.module_->getUniqueName()].multi;

        std::vector<std::shared_ptr<T>> derived_messages;
        for(auto& message : base_messages) {
            derived_messages.push_back(std::static_pointer_cast<T>(message));
        }

        return derived_messages;
    }
} // namespace allpix
