/**
 * @file
 * @brief Implementation of carrier path object
 *
 * @copyright Copyright (c) 2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "CarrierPath.hpp"

#include <map>
#include <numeric>
#include <ostream>
#include <utility>

#include <Math/Point3Dfwd.h>

#if __has_include(<magic_enum/magic_enum.hpp>)
#include <magic_enum/magic_enum.hpp>
#else
#include <magic_enum.hpp>
#endif

#include "objects/DepositedCharge.hpp"
#include "objects/PropagatedCharge.hpp"
#include "objects/exceptions.h"

using namespace allpix;

CarrierPath::CarrierPath(unsigned int charge, CarrierType type, CarrierState state, const DepositedCharge* deposited_charge)
    : charge_(charge), type_(type), state_(state), deposited_charge_(PointerWrapper<DepositedCharge>(deposited_charge)) {}

/**
 * @throws MissingReferenceException If the pointed object is not in scope
 *
 * Object is stored as TRef and can only be accessed if pointed object is in scope
 */
const DepositedCharge* CarrierPath::getDepositedCharge() const {
    auto* deposited_charge = deposited_charge_.get();
    if(deposited_charge == nullptr) {
        throw MissingReferenceException(typeid(*this), typeid(DepositedCharge));
    }
    return deposited_charge;
}

/**
 * @throws MissingReferenceException If the pointed object is not in scope
 *
 * Object is stored as TRef and can only be accessed if pointed object is in scope
 */
const PropagatedCharge* CarrierPath::getPropagatedCharge() const {
    auto* propagated_charge = propagated_charge_.get();
    if(propagated_charge == nullptr) {
        throw MissingReferenceException(typeid(*this), typeid(PropagatedCharge));
    }
    return propagated_charge;
}

void CarrierPath::setPropagatedCharge(const PropagatedCharge* propagated_charge) {
    propagated_charge_ = PointerWrapper<PropagatedCharge>(propagated_charge);
}

double CarrierPath::getDepositionTime() const {
    try {
        return getDepositedCharge()->getLocalTime();
    } catch(MissingReferenceException) {
        if(!points_.empty()) {
            return points_.front().second;
        }
        return 0;
    }
}

void CarrierPath::print(std::ostream& out) const {
    out << "--- Carrier path information\n";
    out << "State: " << magic_enum::enum_name(state_) << "\n";
}

void CarrierPath::loadHistory() {
    deposited_charge_.get();
    propagated_charge_.get();
}

void CarrierPath::petrifyHistory() {
    deposited_charge_.store();
    propagated_charge_.store();
}
