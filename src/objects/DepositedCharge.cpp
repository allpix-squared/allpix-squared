#include "DepositedCharge.hpp"

using namespace allpix;

DepositedCharge::DepositedCharge(ROOT::Math::XYZPoint position,
                                 unsigned int charge,
                                 double event_time,
                                 const MCParticle* mc_particle)
    : SensorCharge(std::move(position), charge, event_time) {
    setMCParticle(mc_particle);
}

const MCParticle* DepositedCharge::getMCParticle() const {
    return dynamic_cast<MCParticle*>(mc_particle_.GetObject());
}

void DepositedCharge::setMCParticle(const MCParticle* mc_particle) {
    mc_particle_ = const_cast<MCParticle*>(mc_particle); // NOLINT
}

ClassImp(DepositedCharge)
