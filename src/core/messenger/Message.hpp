/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MESSAGE_H
#define ALLPIX_MESSAGE_H

#include <vector>

#include "core/geometry/Detector.hpp"

namespace allpix {
    // non-templated base for all message types
    class BaseMessage {
    public:
        // Constructor and destructors
        BaseMessage();
        explicit BaseMessage(std::shared_ptr<Detector> detector);
        virtual ~BaseMessage();

        // Get linked detector
        std::shared_ptr<Detector> getDetector() const;

        // Set default copy behaviour
        BaseMessage(const BaseMessage&) = default;
        BaseMessage& operator=(const BaseMessage&) = default;

    private:
        std::shared_ptr<Detector> detector_;
    };

    // templated version for general messages
    template <typename T> class Message : public BaseMessage {
    public:
        // Constructor to pass the data
        explicit Message(std::vector<T> data);
        Message(std::vector<T> data, std::shared_ptr<Detector> detector);

        // Get the data
        const std::vector<T>& getData() const;

    private:
        std::vector<T> data_;
    };

    // Constructor to pass the data
    template <typename T> Message<T>::Message(std::vector<T> data) : BaseMessage(), data_(std::move(data)) {}
    template <typename T>
    Message<T>::Message(std::vector<T> data, std::shared_ptr<Detector> detector)
        : BaseMessage(detector), data_(std::move(data)) {}

    // Get the data
    template <typename T> const std::vector<T>& Message<T>::getData() const { return data_; }
} // namespace allpix

#endif /* ALLPIX_MESSAGE_H */
