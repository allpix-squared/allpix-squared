/**
 * @file
 * @brief Implementation of deposited charge object
 *
 * @copyright Copyright (c) 2017-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "DepositedCharge.hpp"

#include "objects/exceptions.h"

using namespace allpix;

DepositedCharge::DepositedCharge(ROOT::Math::XYZPoint local_position,
                                 ROOT::Math::XYZPoint global_position,
                                 CarrierType type,
                                 unsigned int charge,
                                 double local_time,
                                 double global_time,
                                 const MCParticle* mc_particle)
    : SensorCharge(std::move(local_position), std::move(global_position), type, charge, local_time, global_time) {
    setMCParticle(mc_particle);
}

/**
 * @throws MissingReferenceException If the pointed object is not in scope
 *
 * Object is stored as TRef and can only be accessed if pointed object is in scope
 */
const MCParticle* DepositedCharge::getMCParticle() const {
    auto* mc_particle = mc_particle_.get();
    if(mc_particle == nullptr) {
        throw MissingReferenceException(typeid(*this), typeid(MCParticle));
    }
    return mc_particle;
}

void DepositedCharge::setMCParticle(const MCParticle* mc_particle) {
    mc_particle_ = PointerWrapper<MCParticle>(mc_particle);
}

void DepositedCharge::print(std::ostream& out) const {
    out << "--- Deposited charge information\n";
    SensorCharge::print(out);
}

void DepositedCharge::loadHistory() { mc_particle_.get(); }
void DepositedCharge::petrifyHistory() { mc_particle_.store(); }
