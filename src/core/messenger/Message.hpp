/**
 * @file
 * @brief Base for the message implementation
 * @copyright MIT License
 */

#ifndef ALLPIX_MESSAGE_H
#define ALLPIX_MESSAGE_H

#include <vector>

#include "core/geometry/Detector.hpp"

namespace allpix {
    /**
     * @brief Type-erased base class for all messages
     *
     * This class can (and should) not be instantiated directly. Deriving from this class is allowed, but in almost all cases
     * instantiating a version of the Message class should be preferred.
     */
    class BaseMessage {
    public:
        /**
         * @brief Essential virtual destructor
         */
        virtual ~BaseMessage();

        ///@{
        /**
         * @brief Use default copy and move behaviour
         */
        BaseMessage(const BaseMessage&) = default;
        BaseMessage& operator=(const BaseMessage&) = default;

        BaseMessage(BaseMessage&&) noexcept = default;
        BaseMessage& operator=(BaseMessage&&) noexcept = default;
        ///@}

        /**
         * @brief Get detector bound to this message
         * @return Linked detector
         */
        std::shared_ptr<Detector> getDetector() const;

    protected:
        /**
         * @brief Construct a general message not linked to a detector
         */
        BaseMessage();
        /**
         * @brief Construct a general message bound to a detector
         * @param detector Linked detector
         */
        explicit BaseMessage(std::shared_ptr<Detector> detector);

    private:
        std::shared_ptr<Detector> detector_;
    };

    /**
     * @brief Generic class for all messages
     *
     * An instantiation of this class should the preferred way to send objects
     */
    template <typename T> class Message : public BaseMessage {
    public:
        /**
         * @brief Constructs a message containing the supplied data
         * @param data List of data objects
         */
        explicit Message(std::vector<T> data);
        /**
         * @brief Constructs a message bound to a detector containing the supplied data
         * @param data List of data objects
         * @param detector Linked detector
         */
        Message(std::vector<T> data, std::shared_ptr<Detector> detector);

        /**
         * @brief Get a reference to the data in this message
         */
        const std::vector<T>& getData() const;

    private:
        std::vector<T> data_;
    };

    // TODO [doc] Should move to an implementation file

    template <typename T> Message<T>::Message(std::vector<T> data) : BaseMessage(), data_(std::move(data)) {}
    template <typename T>
    Message<T>::Message(std::vector<T> data, std::shared_ptr<Detector> detector)
        : BaseMessage(detector), data_(std::move(data)) {}

    template <typename T> const std::vector<T>& Message<T>::getData() const { return data_; }
} // namespace allpix

#endif /* ALLPIX_MESSAGE_H */
