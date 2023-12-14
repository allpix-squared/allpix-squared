/**
 * @file
 * @brief Implementation of propagated charge object
 *
 * @copyright Copyright (c) 2017-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <magic_enum/magic_enum.hpp>
#include <numeric>

#include "PropagatedCharge.hpp"

#include "objects/exceptions.h"

using namespace allpix;

PropagatedCharge::PropagatedCharge(ROOT::Math::XYZPoint local_position,
                                   ROOT::Math::XYZPoint global_position,
                                   CarrierType type,
                                   unsigned int charge,
                                   double local_time,
                                   double global_time,
                                   CarrierState state,
                                   const DepositedCharge* deposited_charge)
    : SensorCharge(std::move(local_position), std::move(global_position), type, charge, local_time, global_time),
      state_(state) {
    deposited_charge_ = PointerWrapper<DepositedCharge>(deposited_charge);
    if(deposited_charge != nullptr) {
        mc_particle_ = deposited_charge->mc_particle_;
    }
}

PropagatedCharge::PropagatedCharge(ROOT::Math::XYZPoint local_position,
                                   ROOT::Math::XYZPoint global_position,
                                   CarrierType type,
                                   std::map<Pixel::Index, Pulse> pulses,
                                   double local_time,
                                   double global_time,
                                   CarrierState state,
                                   const DepositedCharge* deposited_charge)
    : PropagatedCharge(std::move(local_position),
                       std::move(global_position),
                       type,
                       std::accumulate(pulses.begin(),
                                       pulses.end(),
                                       0u,
                                       [](const unsigned int prev, const auto& elem) {
                                           return prev + static_cast<unsigned int>(std::abs(elem.second.getCharge()));
                                       }),
                       local_time,
                       global_time,
                       state,
                       deposited_charge) {
    pulses_ = std::move(pulses); // NOLINT
}

/**
 * @throws MissingReferenceException If the pointed object is not in scope
 *
 * Object is stored as TRef and can only be accessed if pointed object is in scope
 */
const DepositedCharge* PropagatedCharge::getDepositedCharge() const {
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
const MCParticle* PropagatedCharge::getMCParticle() const {
    auto* mc_particle = mc_particle_.get();
    if(mc_particle == nullptr) {
        throw MissingReferenceException(typeid(*this), typeid(MCParticle));
    }
    return mc_particle;
}

std::map<Pixel::Index, Pulse> PropagatedCharge::getPulses() const { return pulses_; }

CarrierState PropagatedCharge::getState() const { return state_; }

void PropagatedCharge::print(std::ostream& out) const {
    out << "--- Propagated charge information\n";
    out << "State: " << magic_enum::enum_name(state_) << "\n";
    SensorCharge::print(out);
}

void PropagatedCharge::loadHistory() {
    deposited_charge_.get();
    mc_particle_.get();
}
void PropagatedCharge::petrifyHistory() {
    deposited_charge_.store();
    mc_particle_.store();
}
