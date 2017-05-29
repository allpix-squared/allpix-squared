namespace allpix {
    template <typename T> Message<T>::Message(std::vector<T> data) : BaseMessage(), data_(std::move(data)) {}
    template <typename T>
    Message<T>::Message(std::vector<T> data, std::shared_ptr<Detector> detector)
        : BaseMessage(detector), data_(std::move(data)) {}

    template <typename T> const std::vector<T>& Message<T>::getData() const { return data_; }

    template <typename T> std::vector<std::reference_wrapper<Object>> Message<T>::getObjectArray() {
        return get_object_array();
    }
    template <typename T>
    template <typename U>
    std::vector<std::reference_wrapper<Object>>
    Message<T>::get_object_array(typename std::enable_if_t<std::is_base_of<Object, U>::value>*) {
        std::vector<std::reference_wrapper<Object>> data(data_.begin(), data_.end());
        return data;
    }
    template <typename T>
    template <typename U>
    std::vector<std::reference_wrapper<Object>>
    Message<T>::get_object_array(typename std::enable_if_t<!std::is_base_of<Object, U>::value>*) {
        return BaseMessage::getObjectArray();
    }
}
