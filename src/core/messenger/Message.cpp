/**
 * @file
 * @brief Implementation of message
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "Message.hpp"

#include <memory>
#include <utility>

#include "core/messenger/exceptions.h"

using namespace allpix;

BaseMessage::BaseMessage() = default;
BaseMessage::BaseMessage(std::shared_ptr<const Detector> detector) : detector_(std::move(detector)) {}
BaseMessage::~BaseMessage() = default;

/**
 * @throws MessageWithoutObjectException If this method is not overridden
 *
 * The override method should return the exact same data but then casted to objects or throw the default exception if this is
 * not possible.
 */
std::vector<std::reference_wrapper<Object>> BaseMessage::getObjectArray() {
    throw MessageWithoutObjectException(typeid(*this));
}
