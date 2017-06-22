/**
 * @file
 * @brief Implementation of message
 *
 * @copyright MIT License
 */

#include "Message.hpp"

#include <memory>
#include <utility>

#include "exceptions.h"

using namespace allpix;

BaseMessage::BaseMessage() = default;
BaseMessage::BaseMessage(std::shared_ptr<const Detector> detector) : detector_(std::move(detector)) {}
BaseMessage::~BaseMessage() = default;

std::shared_ptr<const Detector> BaseMessage::getDetector() const {
    return detector_;
}

/**
 * @throws MessageWithoutObjectException If this method is not overridden
 *
 * The override method should return the exact same data but then casted to objects or throw the default exception if this is
 * not possible.
 */
std::vector<std::reference_wrapper<Object>> BaseMessage::getObjectArray() {
    throw MessageWithoutObjectException(typeid(*this));
}
