/**
 * @file
 * @brief Implementation of propagated charge object
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <numeric>

#include "PropagatedCharge.hpp"

#include "exceptions.h"

using namespace allpix;

PropagatedCharge::PropagatedCharge(ROOT::Math::XYZPoint local_position,
                                   ROOT::Math::XYZPoint global_position,
                                   CarrierType type,
                                   unsigned int charge,
                                   double event_time,
                                   const DepositedCharge* deposited_charge)
    : SensorCharge(std::move(local_position), std::move(global_position), type, charge, event_time) {
    deposited_charge_ref_ = reinterpret_cast<uintptr_t>(deposited_charge); // NOLINT
    if(deposited_charge != nullptr) {
        mc_particle_ref_ = deposited_charge->mc_particle_ref_;
    }
}

PropagatedCharge::PropagatedCharge(ROOT::Math::XYZPoint local_position,
                                   ROOT::Math::XYZPoint global_position,
                                   CarrierType type,
                                   std::map<Pixel::Index, Pulse> pulses,
                                   double event_time,
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
                       event_time,
                       deposited_charge) {
    pulses_ = std::move(pulses);
}

/**
 * @throws MissingReferenceException If the pointed object is not in scope
 *
 * Object is stored as TRef and can only be accessed if pointed object is in scope
 */
const DepositedCharge* PropagatedCharge::getDepositedCharge() const {
    auto deposited_charge = reinterpret_cast<DepositedCharge*>(deposited_charge_ref_);
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
    auto mc_particle = reinterpret_cast<MCParticle*>(mc_particle_ref_);
    if(mc_particle == nullptr) {
        throw MissingReferenceException(typeid(*this), typeid(MCParticle));
    }
    return mc_particle;
}

std::map<Pixel::Index, Pulse> PropagatedCharge::getPulses() const {
    return pulses_;
}

void PropagatedCharge::print(std::ostream& out) const {
    out << "--- Propagated charge information\n";
    SensorCharge::print(out);
}

void PropagatedCharge::storeHistory() {
    std::cout << "Storing PropagatedCharge " << this << std::endl;
    std::cout << "\t DepositedCharge:" << std::endl;
    std::cout << "\t uintptr_t     0x" << std::hex << deposited_charge_ref_ << std::dec << std::endl;
    std::cout << "\t ptr           " << reinterpret_cast<DepositedCharge*>(deposited_charge_ref_) << std::endl;
    deposited_charge_ = TRef(reinterpret_cast<DepositedCharge*>(deposited_charge_ref_));
    std::cout << "\t TRef resolved " << deposited_charge_.GetObject() << std::endl;
    deposited_charge_ref_ = 0;

    mc_particle_ = TRef(reinterpret_cast<MCParticle*>(mc_particle_ref_));
    mc_particle_ref_ = 0;
}

void PropagatedCharge::loadHistory() {
    std::cout << "Loading PropagatedCharge " << this << std::endl;
    std::cout << "\t DepositedCharge:" << std::endl;
    std::cout << "\t TRef resolved " << deposited_charge_.GetObject() << std::endl;
    deposited_charge_ref_ = reinterpret_cast<uintptr_t>(deposited_charge_.GetObject());

    std::cout << "\t uintptr_t     0x" << std::hex << deposited_charge_ref_ << std::dec << std::endl;
    std::cout << "\t ptr           " << reinterpret_cast<DepositedCharge*>(deposited_charge_ref_) << std::endl;

    mc_particle_ref_ = reinterpret_cast<uintptr_t>(mc_particle_.GetObject());
    // FIXME we need to reset TRef member
}
