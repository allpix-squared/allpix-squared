/**
 * @file
 * @brief Implementation of Monte-Carlo track object
 * @copyright Copyright (c) 2018 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "MCTrack.hpp"

using namespace allpix;

MCTrack::MCTrack(ROOT::Math::XYZPoint start_point,
                 std::string g4_volume,
                 std::string g4_prod_process_name,
                 int g4_prod_process_type,
                 int particle_id,
                 int track_id,
                 double initial_kin_E,
                 double initial_tot_E)
    : start_point_(std::move(start_point)), origin_g4_vol_name_(std::move(g4_volume)),
      origin_g4_process_name_(std::move(g4_prod_process_name)), origin_g4_process_type_(g4_prod_process_type),
      particle_id_(particle_id), track_id_(track_id), initial_kin_E_(initial_kin_E), initial_tot_E_(initial_tot_E) {
    setParent(nullptr);
}

ROOT::Math::XYZPoint MCTrack::getStartPoint() const {
    return start_point_;
}

ROOT::Math::XYZPoint MCTrack::getEndPoint() const {
    return end_point_;
}

int MCTrack::getParticleID() const {
    return particle_id_;
}

int MCTrack::getTrackID() const {
    return track_id_;
}

int MCTrack::getCreationProcessType() const {
    return origin_g4_process_type_;
}

int MCTrack::getNumberOfSteps() const {
    return n_steps_;
}

double MCTrack::getKineticEnergyInitial() const {

    return initial_kin_E_;
}

double MCTrack::getTotalEnergyInitial() const {
    return initial_tot_E_;
}

double MCTrack::getKineticEnergyFinal() const {
    return final_kin_E_;
}

double MCTrack::getTotalEnergyFinal() const {
    return final_tot_E_;
}

std::string MCTrack::getOriginatingVolumeName() const {
    return origin_g4_vol_name_;
}

std::string MCTrack::getCreationProcessName() const {
    return origin_g4_process_name_;
}

/**
 * Object is stored as TRef and can only be accessed if pointed object is in scope
 */
const MCTrack* MCTrack::getParent() const {
    return dynamic_cast<MCTrack*>(parent_.GetObject());
}

void MCTrack::setEndPoint(ROOT::Math::XYZPoint end_point) {
    end_point_ = std::move(end_point);
}

void MCTrack::setNumberOfSteps(int n_steps) {
    n_steps_ = n_steps;
}

void MCTrack::setKineticEnergyFinal(double final_kin_E) {
    final_kin_E_ = final_kin_E;
}

void MCTrack::setTotalEnergyFinal(double final_tot_E) {
    final_tot_E_ = final_tot_E;
}

void MCTrack::setParent(const MCTrack* mc_track) {
    parent_ = const_cast<MCTrack*>(mc_track); // NOLINT
}

void MCTrack::print(std::ostream& out) const {
    auto parent = getParent();
    out << "\n ------- Printing MCTrack information for track: " << track_id_ << " (" << this << ") -------\n"
        << "Particle type (PDG ID): " << particle_id_ << '\n'
        << "Production process: " << origin_g4_process_name_ << " (G4 process type: " << origin_g4_process_type_ << ")\n"
        << "Production in G4Volume: " << origin_g4_vol_name_ << '\n'
        << std::left << std::setw(25) << "Initial position:" << std::right << std::setw(10) << start_point_.X() << " mm |"
        << std::right << std::setw(10) << start_point_.Y() << " mm |" << std::right << std::setw(10) << start_point_.Z()
        << " mm\n"
        << std::left << std::setw(25) << "Final position:" << std::right << std::setw(10) << end_point_.X() << " mm |"
        << std::setw(10) << end_point_.Y() << " mm |" << std::setw(10) << end_point_.Z() << " mm\n"
        << "Initial kinetic energy: " << initial_kin_E_ << " MeV \t Final kinetic energy: " << final_kin_E_ << " MeV\n"
        << "Initial total energy: " << initial_tot_E_ << " MeV \t Final total energy: " << final_tot_E_ << " MeV\n";
    if(parent != nullptr) {
        out << "Linked parent: " << parent_.GetObject() << " Parent track ID: " << parent->getTrackID() << '\n';
    } else {
        out << "Linked parent: <nullptr>";
    }
    out << " -----------------------------------------------------------------\n";
}

ClassImp(MCTrack)
