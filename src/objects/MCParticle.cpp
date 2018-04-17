/**
 * @file
 * @brief Implementation of Monte-Carlo particle object
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "MCParticle.hpp"

using namespace allpix;

MCParticle::MCParticle(ROOT::Math::XYZPoint local_start_point,
                       ROOT::Math::XYZPoint global_start_point,
                       ROOT::Math::XYZPoint local_end_point,
                       ROOT::Math::XYZPoint global_end_point,
                       int particle_id,
                       int track_id)
    : local_start_point_(std::move(local_start_point)), global_start_point_(std::move(global_start_point)),
      local_end_point_(std::move(local_end_point)), global_end_point_(std::move(global_end_point)),
      particle_id_(particle_id), track_id_(track_id) {
    setParent(nullptr);
}

ROOT::Math::XYZPoint MCParticle::getLocalStartPoint() const {
    return local_start_point_;
}
ROOT::Math::XYZPoint MCParticle::getGlobalStartPoint() const {
    return global_start_point_;
}

ROOT::Math::XYZPoint MCParticle::getLocalEndPoint() const {
    return local_end_point_;
}
ROOT::Math::XYZPoint MCParticle::getGlobalEndPoint() const {
    return global_end_point_;
}

int MCParticle::getParticleID() const {
    return particle_id_;
}

int MCParticle::getTrackID() const {
    return track_id_;
}

/**
 * Object is stored as TRef and can only be accessed if pointed object is in scope
 */
const MCParticle* MCParticle::getParent() const {
    return dynamic_cast<MCParticle*>(parent_.GetObject());
}

void MCParticle::setParent(const MCParticle* mc_particle) {
    parent_ = const_cast<MCParticle*>(mc_particle); // NOLINT
}

void MCParticle::print(std::ostream& out) const {
    out << "\n ------- Printing particle information (" << this << ")-------\n"
        << "Particle type (PDG ID): " << particle_id_ << " | Belongs to track: " << track_id_ << '\n'
        << "Local start point:\t " << local_start_point_.X() << " mm\t|" << local_start_point_.Y() << " mm\t|"
        << local_start_point_.Z() << " mm\n"
        << "Global start point:\t " << global_start_point_.X() << " mm\t|" << global_start_point_.Y() << " mm\t|"
        << global_start_point_.Z() << " mm\n"
        << "Local end point: \t " << local_end_point_.X() << " mm\t|" << local_end_point_.Y() << " mm\t|"
        << local_end_point_.Z() << " mm\n"
        << "Global end point: \t " << global_end_point_.X() << " mm\t|" << global_end_point_.Y() << " mm\t|"
        << global_end_point_.Z() << " mm\n"
        << "Parent particle: " << parent_.GetObject() << '\n'
        << " -----------------------------------------------------------------\n";
}

ClassImp(MCParticle)
