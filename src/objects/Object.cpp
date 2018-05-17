/**
 * @file
 * @brief Implementation of Object base class
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Object.hpp"

using namespace allpix;

std::ostream& allpix::operator<<(std::ostream& out, const Object& obj) {
    obj.print(out);
    return out;
}

Object::Object(const Object&) = default;
Object& Object::operator=(const Object&) = default;

Object::Object(Object&&) = default;
Object& Object::operator=(Object&&) = default;
