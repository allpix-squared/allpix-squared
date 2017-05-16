/**
 * @file
 * @brief Implementation of message
 *
 * @copyright MIT License
 */

#include "Message.hpp"

#include <memory>
#include <utility>

using namespace allpix;

BaseMessage::BaseMessage() = default;
BaseMessage::BaseMessage(std::shared_ptr<Detector> detector) : detector_(std::move(detector)) {}
BaseMessage::~BaseMessage() = default;

std::shared_ptr<Detector> BaseMessage::getDetector() const {
    return detector_;
}
