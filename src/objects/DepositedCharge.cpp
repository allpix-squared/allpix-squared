/**
 * @file
 * @brief Implementation of deposited charge object
 * @copyright MIT License
 */

#include "DepositedCharge.hpp"

using namespace allpix;

DepositedCharge::DepositedCharge(ROOT::Math::XYZPoint local_position,
                                 ROOT::Math::XYZPoint global_position,
                                 unsigned int charge,
                                 double event_time,
                                 const MCParticle* mc_particle)
    : SensorCharge(std::move(local_position), std::move(global_position), charge, event_time) {
    setMCParticle(mc_particle);
}

/**
 * Object is stored as TRef and can only be accessed if pointed object is in scope
 */
const MCParticle* DepositedCharge::getMCParticle() const {
    return dynamic_cast<MCParticle*>(mc_particle_.GetObject());
}

void DepositedCharge::setMCParticle(const MCParticle* mc_particle) {
    mc_particle_ = const_cast<MCParticle*>(mc_particle); // NOLINT
}

ClassImp(DepositedCharge)
