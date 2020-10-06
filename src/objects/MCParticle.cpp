/**
 * @file
 * @brief Implementation of Monte-Carlo particle object
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "MCParticle.hpp"
#include <sstream>

#include <Math/Vector3D.h>

using namespace allpix;

MCParticle::MCParticle(ROOT::Math::XYZPoint local_start_point,
                       ROOT::Math::XYZPoint global_start_point,
                       ROOT::Math::XYZPoint local_end_point,
                       ROOT::Math::XYZPoint global_end_point,
                       int particle_id,
                       double time)
    : local_start_point_(std::move(local_start_point)), global_start_point_(std::move(global_start_point)),
      local_end_point_(std::move(local_end_point)), global_end_point_(std::move(global_end_point)),
      particle_id_(particle_id), global_time_(time) {
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

ROOT::Math::XYZPoint MCParticle::getLocalReferencePoint() const {
    // Direction for parametric equation of line through start/end points
    auto direction =
        static_cast<ROOT::Math::XYZVector>(local_end_point_) - static_cast<ROOT::Math::XYZVector>(local_start_point_);

    if(direction.z() != 0) {
        // Calculate parameter for line intersection with plane at z = 0, local coordinates
        auto t = -1 * local_start_point_.z() / direction.z();
        // Calculate reference point at z = 0 from parametric line equation
        return (direction * t + local_start_point_);
    } else {
        // Both points are coplanar with x-y plane. SImply return their center:
        return (static_cast<ROOT::Math::XYZVector>(local_end_point_) + local_start_point_) / 2.0;
    }
}

int MCParticle::getParticleID() const {
    return particle_id_;
}

double MCParticle::getGlobalTime() const {
    return global_time_;
}

double MCParticle::getLocalTime() const {
    return local_time_;
}

void MCParticle::setParent(const MCParticle* mc_particle) {
    parent_ = const_cast<MCParticle*>(mc_particle); // NOLINT

    // Update the local time with the arrival time of the primary particle:
    if(mc_particle != nullptr) {
        local_time_ = global_time_ - getPrimary()->getGlobalTime();
    }
}

/**
 * Object is stored as TRef and can only be accessed if pointed object is in scope
 */
const MCParticle* MCParticle::getParent() const {
    return dynamic_cast<MCParticle*>(parent_.GetObject());
}

/**
 * Object is stored as TRef and can only be accessed if pointed object is in scope
 */
const MCParticle* MCParticle::getPrimary() const {
    auto parent = dynamic_cast<MCParticle*>(parent_.GetObject());
    return (parent == nullptr ? this : parent->getPrimary());
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
    static const size_t big_gap = 25;
    static const size_t med_gap = 10;
    static const size_t small_gap = 6;
    static const size_t largest_output = big_gap + 3 * med_gap + 3 * small_gap;

    auto track = getTrack();
    auto parent = getParent();

    auto title = std::stringstream();
    title << "--- Printing MCParticle information (" << this << ") ";
    out << '\n'
        << std::setw(largest_output) << std::left << std::setfill('-') << title.str() << '\n'
        << std::setfill(' ') << std::left << std::setw(big_gap) << "Particle type (PDG ID): " << std::right
        << std::setw(small_gap) << particle_id_ << '\n'
        << std::left << std::setw(big_gap) << "Local start point:" << std::right << std::setw(med_gap)
        << local_start_point_.X() << std::setw(small_gap) << " mm |" << std::setw(med_gap) << local_start_point_.Y()
        << std::setw(small_gap) << " mm |" << std::setw(med_gap) << local_start_point_.Z() << std::setw(small_gap)
        << " mm  \n"
        << std::left << std::setw(big_gap) << "Global start point:" << std::right << std::setw(med_gap)
        << global_start_point_.X() << std::setw(small_gap) << " mm |" << std::setw(med_gap) << global_start_point_.Y()
        << std::setw(small_gap) << " mm |" << std::setw(med_gap) << global_start_point_.Z() << std::setw(small_gap)
        << " mm  \n"
        << std::left << std::setw(big_gap) << "Local end point:" << std::right << std::setw(med_gap) << local_end_point_.X()
        << std::setw(small_gap) << " mm |" << std::setw(med_gap) << local_end_point_.Y() << std::setw(small_gap) << " mm |"
        << std::setw(med_gap) << local_end_point_.Z() << std::setw(small_gap) << " mm  \n"
        << std::left << std::setw(big_gap) << "Global end point:" << std::right << std::setw(med_gap)
        << global_end_point_.X() << std::setw(small_gap) << " mm |" << std::setw(med_gap) << global_end_point_.Y()
        << std::setw(small_gap) << " mm |" << std::setw(med_gap) << global_end_point_.Z() << std::setw(small_gap)
        << " mm  \n"
        << std::left << std::setw(big_gap) << "Linked parent:";
    if(parent != nullptr) {
        out << std::right << std::setw(small_gap) << parent << '\n';
    } else {
        out << std::right << std::setw(small_gap) << "<nullptr>\n";
    }
    out << std::left << std::setw(big_gap) << "Linked track:";
    if(track != nullptr) {
        out << std::right << std::setw(small_gap) << track << '\n';
    } else {
        out << std::right << std::setw(small_gap) << "<nullptr>\n";
    }
    out << std::setfill('-') << std::setw(largest_output) << "" << std::setfill(' ') << std::endl;
}
