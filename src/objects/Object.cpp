/**
 * @file
 * @brief Implementation of Object base class
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "Object.hpp"

using namespace allpix;

std::ostream& allpix::operator<<(std::ostream& out, const Object& obj) {
    obj.print(out);
    return out;
}

bool operator<(const TRef& ref1, const TRef& ref2) {
    if(ref1.GetPID() == ref2.GetPID()) {
        return ref1.GetUniqueID() < ref2.GetUniqueID();
    }
    return ref1.GetPID() < ref2.GetPID();
}
