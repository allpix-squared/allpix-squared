/**
 * @file
 * @brief Implementation of deposited charge object
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "DepositedCharge.hpp"

#include "exceptions.h"

using namespace allpix;

DepositedCharge::DepositedCharge(ROOT::Math::XYZPoint local_position,
                                 ROOT::Math::XYZPoint global_position,
                                 CarrierType type,
                                 unsigned int charge,
                                 double event_time,
                                 const MCParticle* mc_particle)
    : SensorCharge(std::move(local_position), std::move(global_position), type, charge, event_time) {
    setMCParticle(mc_particle);
}

/**
 * @throws MissingReferenceException If the pointed object is not in scope
 *
 * Object is stored as TRef and can only be accessed if pointed object is in scope
 */
const MCParticle* DepositedCharge::getMCParticle() const {
    auto mc_particle = reinterpret_cast<MCParticle*>(mc_particle_ref_);
    if(mc_particle == nullptr) {
        throw MissingReferenceException(typeid(*this), typeid(MCParticle));
    }
    return mc_particle;
}

void DepositedCharge::setMCParticle(const MCParticle* mc_particle) {
    mc_particle_ref_ = reinterpret_cast<uintptr_t>(mc_particle); // NOLINT
}

void DepositedCharge::print(std::ostream& out) const {
    out << "--- Deposited charge information\n";
    SensorCharge::print(out);
}

void DepositedCharge::storeHistory() {
    std::cout << "Storing DepositedCharge " << this << " type " << (getType() == CarrierType::ELECTRON ? "\"e\"" : "\"h\"")
              << std::endl;
    std::cout << "\t MCParticle:" << std::endl;
    std::cout << "\t uintptr_t     0x" << std::hex << mc_particle_ref_ << std::dec << std::endl;
    std::cout << "\t ptr           " << reinterpret_cast<MCParticle*>(mc_particle_ref_) << std::endl;
    mc_particle_ = TRef(reinterpret_cast<MCParticle*>(mc_particle_ref_));
    std::cout << "\t TRef resolved " << mc_particle_.GetObject() << std::endl;

    mc_particle_ref_ = 0;
}

void DepositedCharge::loadHistory() {
    std::cout << "Loading DepositedCharge " << this << std::endl;
    std::cout << "\t MCParticle:" << std::endl;
    std::cout << "\t TRef resolved " << mc_particle_.GetObject() << std::endl;
    mc_particle_ref_ = reinterpret_cast<uintptr_t>(mc_particle_.GetObject());

    std::cout << "\t uintptr_t     0x" << std::hex << mc_particle_ref_ << std::dec << std::endl;
    std::cout << "\t ptr           " << reinterpret_cast<MCParticle*>(mc_particle_ref_) << std::endl;

    // FIXME we need to reset TRef member
}
