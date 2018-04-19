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
                       int particle_id)
    : local_start_point_(std::move(local_start_point)), global_start_point_(std::move(global_start_point)),
      local_end_point_(std::move(local_end_point)), global_end_point_(std::move(global_end_point)),
      particle_id_(particle_id) {
    setParent(nullptr);
    setTrack(nullptr);
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

void MCParticle::setParent(const MCParticle* mc_particle) {
    parent_ = const_cast<MCParticle*>(mc_particle); // NOLINT
}

/**
 * Object is stored as TRef and can only be accessed if pointed object is in scope
 */
const MCParticle* MCParticle::getParent() const {
    return dynamic_cast<MCParticle*>(parent_.GetObject());
}

void MCParticle::setTrack(const MCTrack* mc_track) {
    track_ = const_cast<MCTrack*>(mc_track); // NOLINT
}

/**
 * Object is stored as TRef and can only be accessed if pointed object is in scope
 */
const MCTrack* MCParticle::getTrack() const {
    return dynamic_cast<MCTrack*>(track_.GetObject());
}

void MCParticle::print(std::ostream& out) const {
    auto track = getTrack();
    auto parent = getTrack();
    out << "\n ------- Printing MCParticle information (" << this << ")-------\n"
        << "Particle type (PDG ID): " << particle_id_ << '\n'
        << std::left << std::setw(25) << "Local start point:" << std::right << std::setw(10) << local_start_point_.X()
        << " mm |" << std::setw(10) << local_start_point_.Y() << " mm |" << std::setw(10) << local_start_point_.Z()
        << " mm\n"
        << std::left << std::setw(25) << "Global start point:" << std::right << std::setw(10) << global_start_point_.X()
        << " mm |" << std::setw(10) << global_start_point_.Y() << " mm |" << std::setw(10) << global_start_point_.Z()
        << " mm\n"
        << std::left << std::setw(25) << "Local end point:" << std::right << std::setw(10) << local_end_point_.X() << " mm |"
        << std::setw(10) << local_end_point_.Y() << " mm |" << std::setw(10) << local_end_point_.Z() << " mm\n"
        << std::left << std::setw(25) << "Global end point:" << std::right << std::setw(10) << global_end_point_.X()
        << " mm |" << std::setw(10) << global_end_point_.Y() << " mm |" << std::setw(10) << global_end_point_.Z() << " mm\n";
    if(parent != nullptr) {
        out << "Linked parent: " << parent << '\n';
    } else {
        out << "Linked parent: <nullptr> \n";
    }
    if(track != nullptr) {
        out << "Linked track: " << track << " (ID: " << track->getTrackID() << ")\n";
    } else {
        out << "Linked track: <nullptr> \n";
    }
    out << " -----------------------------------------------------------------\n";
}

ClassImp(MCParticle)
