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
                       ROOT::Math::XYZPoint global_end_point)
    : local_start_point_(std::move(local_start_point)), global_start_point_(std::move(global_start_point)),
      local_end_point_(std::move(local_end_point)), global_end_point_(std::move(global_end_point)) {
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

/**
 * Object is stored as TRef and can only be accessed if pointed object is in scope
 */
const MCTrack* MCParticle::getParent() const {
    return dynamic_cast<MCTrack*>(parent_.GetObject());
}

void MCParticle::setParent(const MCTrack* mc_particle) {
    parent_ = const_cast<MCTrack*>(mc_particle); // NOLINT
}

void MCParticle::print(std::ostream& out) const {
    auto parent = getParent();
    out << "\n ------- Printing MCParticle information (" << this << ")-------\n"
        << "Particle type (PDG ID): " << parent->getParticleID() << " | Belongs to track: " << parent->getTrackID() << '\n'
        << "Local start point:\t " << local_start_point_.X() << " mm\t|" << local_start_point_.Y() << " mm\t|"
        << local_start_point_.Z() << " mm\n"
        << "Global start point:\t " << global_start_point_.X() << " mm\t|" << global_start_point_.Y() << " mm\t|"
        << global_start_point_.Z() << " mm\n"
        << "Local end point: \t " << local_end_point_.X() << " mm\t|" << local_end_point_.Y() << " mm\t|"
        << local_end_point_.Z() << " mm\n"
        << "Global end point: \t " << global_end_point_.X() << " mm\t|" << global_end_point_.Y() << " mm\t|"
        << global_end_point_.Z() << " mm\n"
        << "Parent track: " << parent_.GetObject() << '\n'
        << " -----------------------------------------------------------------\n";
}

ClassImp(MCParticle)
