/**
 * @file
 * @brief Implementation of Monte-Carlo track object
 * @copyright Copyright (c) 2018-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "MCTrack.hpp"
#include <sstream>

using namespace allpix;

MCTrack::MCTrack(ROOT::Math::XYZPoint start_point,
                 ROOT::Math::XYZPoint end_point,
                 std::string g4_volume,
                 std::string g4_prod_process_name,
                 int g4_prod_process_type,
                 int particle_id,
                 double initial_kin_E,
                 double final_kin_E,
                 double initial_tot_E,
                 double final_tot_E)
    : start_point_(std::move(start_point)), end_point_(std::move(end_point)), origin_g4_vol_name_(std::move(g4_volume)),
      origin_g4_process_name_(std::move(g4_prod_process_name)), origin_g4_process_type_(g4_prod_process_type),
      particle_id_(particle_id), initial_kin_E_(initial_kin_E), final_kin_E_(final_kin_E), initial_tot_E_(initial_tot_E),
      final_tot_E_(final_tot_E) {
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

int MCTrack::getCreationProcessType() const {
    return origin_g4_process_type_;
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
    return parent_.get();
}

void MCTrack::setParent(const MCTrack* mc_track) {
    parent_ = PointerWrapper<MCTrack>(mc_track);
}

void MCTrack::print(std::ostream& out) const {
    static const size_t big_gap = 25;
    static const size_t med_gap = 10;
    static const size_t small_gap = 6;
    static const size_t largest_output = 2 * big_gap + 2 * med_gap + 2 * small_gap;

    const auto* parent = getParent();

    auto title = std::stringstream();
    title << "--- Printing MCTrack information for track (" << this << ") ";

    out << '\n'
        << std::setw(largest_output) << std::left << std::setfill('-') << title.str() << '\n'
        << std::setfill(' ') << std::left << std::setw(big_gap) << "Particle type (PDG ID): " << std::right
        << std::setw(small_gap) << particle_id_ << '\n'
        << std::left << std::setw(big_gap) << "Production process: " << std::right << std::setw(small_gap)
        << origin_g4_process_name_ << " (G4 process type: " << origin_g4_process_type_ << ")\n"
        << std::left << std::setw(big_gap) << "Production in G4Volume: " << std::right << std::setw(small_gap)
        << origin_g4_vol_name_ << '\n'
        << std::left << std::setw(big_gap) << "Initial position:" << std::right << std::setw(med_gap) << start_point_.X()
        << " mm | " << std::setw(med_gap) << start_point_.Y() << " mm | " << std::setw(med_gap) << start_point_.Z()
        << " mm\n"
        << std::left << std::setw(big_gap) << "Final position:" << std::right << std::setw(med_gap) << end_point_.X()
        << " mm | " << std::setw(med_gap) << end_point_.Y() << " mm | " << std::setw(med_gap) << end_point_.Z() << " mm\n"
        << std::left << std::setw(big_gap) << "Initial kinetic energy: " << std::right << std::setw(med_gap)
        << initial_kin_E_ << std::setw(small_gap) << " MeV | " << std::left << std::setw(big_gap)
        << "Final kinetic energy: " << std::right << std::setw(med_gap) << final_kin_E_ << std::setw(small_gap)
        << " MeV   \n"
        << std::left << std::setw(big_gap) << "Initial total energy: " << std::right << std::setw(med_gap) << initial_tot_E_
        << std::setw(small_gap) << " MeV | " << std::left << std::setw(big_gap) << "Final total energy: " << std::right
        << std::setw(med_gap) << final_tot_E_ << std::setw(small_gap) << " MeV   \n";
    if(parent != nullptr) {
        out << "Linked parent: " << parent << '\n';
    } else {
        out << "Linked parent: <nullptr>\n";
    }
    out << std::setfill('-') << std::setw(largest_output) << "" << std::setfill(' ') << std::endl;
}

void MCTrack::loadHistory() {
    parent_.get();
}
void MCTrack::petrifyHistory() {
    parent_.store();
}
