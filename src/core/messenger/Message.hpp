/**
 * @file
 * @brief Base for the message implementation
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MESSAGE_H
#define ALLPIX_MESSAGE_H

#include <vector>

#include "core/geometry/Detector.hpp"
#include "objects/Object.hpp"

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

        /// @{
        /**
         * @brief Use default copy and move behaviour
         */
        BaseMessage(const BaseMessage&) = default;
        BaseMessage& operator=(const BaseMessage&) = default;

        BaseMessage(BaseMessage&&) noexcept = default;
        BaseMessage& operator=(BaseMessage&&) noexcept = default;
        /// @}

        /**
         * @brief Get detector bound to this message
         * @return Linked detector
         */
        std::shared_ptr<const Detector> getDetector() const { return detector_; }

        /**
         * @brief Get list of objects stored in this message if possible
         * @return Array of base objects
         */
        virtual std::vector<std::reference_wrapper<Object>> getObjectArray();

    protected:
        /**
         * @brief Construct a general message not linked to a detector
         */
        BaseMessage();
        /**
         * @brief Construct a general message bound to a detector
         * @param detector Linked detector
         */
        explicit BaseMessage(std::shared_ptr<const Detector> detector);

    private:
        std::shared_ptr<const Detector> detector_;
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
        Message(std::vector<T> data, const std::shared_ptr<const Detector>& detector);

        /**
         * @brief Destructs messages, skipping optional cleanup of objects
         */
        ~Message() override;

        /**
         * @brief Get a reference to the data in this message
         */
        const std::vector<T>& getData() const;

        /**
         * @brief Get data as list of objects if the contents can be converted
         * @return Data as list of object references (throws if not possible)
         */
        std::vector<std::reference_wrapper<Object>> getObjectArray() override;

    private:
        /**
         * @brief Returns object array for messages containing objects
         */
        template <typename U = T>
        std::vector<std::reference_wrapper<Object>>
        get_object_array(typename std::enable_if<std::is_base_of<Object, U>::value>::type* = nullptr);

        /**
         * @brief Throws error if message does not contain object
         */
        template <typename U = T>
        std::vector<std::reference_wrapper<Object>>
        get_object_array(typename std::enable_if<!std::is_base_of<Object, U>::value>::type* = nullptr);

        /**
         * @brief Set kMustCleanup bit in all Objects to false, to prevent cleanup by ROOT
         */
        template <typename U = T>
        void skip_object_cleanup(typename std::enable_if<std::is_base_of<Object, U>::value>::type* = nullptr);

        /**
         * @brief Does nothing if message does not contain objects
         */
        template <typename U = T>
        void skip_object_cleanup(typename std::enable_if<!std::is_base_of<Object, U>::value>::type* = nullptr) {}

        std::vector<T> data_;
    };
} // namespace allpix

// Include template members
#include "Message.tpp"

#endif /* ALLPIX_MESSAGE_H */
