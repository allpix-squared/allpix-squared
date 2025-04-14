/**
 * @file
 * @brief Implementation of Object base class
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "Object.hpp"

using namespace corryvreckan;

Object::Object() = default;
Object::Object(std::string detectorID) : m_detectorID(std::move(detectorID)) {}
Object::Object(double timestamp) : m_timestamp(timestamp) {}
Object::Object(std::string detectorID, double timestamp) : m_detectorID(std::move(detectorID)), m_timestamp(timestamp) {}
Object::Object(const Object&) = default;
Object::~Object() = default;

std::ostream& corryvreckan::operator<<(std::ostream& out, const Object& obj) {
    obj.print(out);
    return out;
}
