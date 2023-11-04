/**
 * @file
 * @brief Implementation of Monte-Carlo particle object
 *
 * @copyright Copyright (c) 2017-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
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
                       double local_time,
                       double global_time)
    : local_start_point_(std::move(local_start_point)), global_start_point_(std::move(global_start_point)),
      local_end_point_(std::move(local_end_point)), global_end_point_(std::move(global_end_point)),
      particle_id_(particle_id), local_time_(local_time), global_time_(global_time) {
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

double MCParticle::getTotalEnergyArrival() const {
    return total_energy_arrival_;
}

void MCParticle::setTotalEnergyArrival(double total_energy) {
    total_energy_arrival_ = total_energy;
}

double MCParticle::getKineticEnergyArrival() const {
    return kinetic_energy_arrival_;
}

void MCParticle::setKineticEnergyArrival(double kinetic_energy) {
    kinetic_energy_arrival_ = kinetic_energy;
}

unsigned int MCParticle::getTotalDepositedCharge() const {
    return deposited_charge_;
}

void MCParticle::setTotalDepositedCharge(unsigned int total_charge) {
    deposited_charge_ = total_charge;
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
    parent_ = PointerWrapper<MCParticle>(mc_particle);
}

/**
 * Object is stored as \ref allpix::Object::PointerWrapper and can only be accessed if pointed object is in scope
 */
const MCParticle* MCParticle::getParent() const {
    return parent_.get();
}

/**
 * Object is stored as \ref allpix::Object::PointerWrapper and can only be accessed if pointed object is in scope
 */
bool MCParticle::isPrimary() const {
    return (parent_.get() == nullptr);
}

/**
 * Object is stored as \ref allpix::Object::PointerWrapper and can only be accessed if pointed object is in scope
 */
const MCParticle* MCParticle::getPrimary() const { // NOLINT
    auto* parent = parent_.get();
    return (parent == nullptr ? this : parent->getPrimary());
}

void MCParticle::setTrack(const MCTrack* mc_track) {
    track_ = PointerWrapper<MCTrack>(mc_track);
}

/**
 * Object is stored as \ref allpix::Object::PointerWrapper and can only be accessed if pointed object is in scope
 */
const MCTrack* MCParticle::getTrack() const {
    return track_.get();
}

void MCParticle::print(std::ostream& out) const {
    static const size_t big_gap = 25;
    static const size_t med_gap = 10;
    static const size_t small_gap = 6;
    static const size_t largest_output = big_gap + 3 * med_gap + 3 * small_gap;

    const auto* track = getTrack();
    const auto* parent = getParent();

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
        << std::left << std::setw(big_gap) << "Local time:" << std::right << std::setw(med_gap) << local_time_
        << std::setw(small_gap) << " ns \n"
        << std::left << std::setw(big_gap) << "Global time:" << std::right << std::setw(med_gap) << global_time_
        << std::setw(small_gap) << " ns \n"
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

void MCParticle::loadHistory() {
    parent_.get();
    track_.get();
}
void MCParticle::petrifyHistory() {
    parent_.store();
    track_.store();
}
