/**
 * @file
 * @brief Implementation of Monte-Carlo particle object
 * @copyright MIT License
 */

#include "MCParticle.hpp"

using namespace allpix;

MCParticle::MCParticle(ROOT::Math::XYZPoint local_entry_point,
                       ROOT::Math::XYZPoint global_entry_point,
                       ROOT::Math::XYZPoint local_exit_point,
                       ROOT::Math::XYZPoint global_exit_point,
                       int particle_id)
    : local_entry_point_(std::move(local_entry_point)), global_entry_point_(std::move(global_entry_point)),
      local_exit_point_(std::move(local_exit_point)), global_exit_point_(std::move(global_exit_point)),
      particle_id_(particle_id) {}

ROOT::Math::XYZPoint MCParticle::getLocalEntryPoint() const {
    return local_entry_point_;
}
ROOT::Math::XYZPoint MCParticle::getGlobalEntryPoint() const {
    return global_entry_point_;
}

ROOT::Math::XYZPoint MCParticle::getLocalExitPoint() const {
    return local_exit_point_;
}
ROOT::Math::XYZPoint MCParticle::getGlobalExitPoint() const {
    return global_exit_point_;
}

int MCParticle::getParticleID() const {
    return particle_id_;
}

ClassImp(MCParticle)
