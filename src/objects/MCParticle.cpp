#include "MCParticle.hpp"

using namespace allpix;

MCParticle::MCParticle(ROOT::Math::XYZPoint entry_point, ROOT::Math::XYZPoint exit_point, int particle_id)
    : entry_point_(std::move(entry_point)), exit_point_(std::move(exit_point)), particle_id_(particle_id) {}

ROOT::Math::XYZPoint MCParticle::getEntryPoint() const {
    return entry_point_;
}

ROOT::Math::XYZPoint MCParticle::getExitPoint() const {
    return exit_point_;
}

int MCParticle::getParticleID() const {
    return particle_id_;
}

ClassImp(MCParticle)
