/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MESSAGE_H
#define ALLPIX_MESSAGE_H

#include <vector>

#include "BaseMessage.hpp"
#include "core/geometry/Detector.hpp"

namespace allpix {
    template <typename T> class Message : public BaseMessage {
    public:
        // Constructor to pass the data
        Message(std::vector<T> data);
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
}

#endif /* ALLPIX_MESSAGE_H */
