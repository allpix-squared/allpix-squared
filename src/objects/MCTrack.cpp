/**
 * @file
 * @brief Implementation of Monte-Carlo track object
 *
 * @copyright Copyright (c) 2018-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "MCTrack.hpp"

#include <cstddef>
#include <iomanip>
#include <ios>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>

#include <Math/Point3Dfwd.h>
#include <Math/Vector3Dfwd.h>

using namespace allpix;

MCTrack::MCTrack(ROOT::Math::XYZPoint start_point,
                 ROOT::Math::XYZPoint end_point,
                 std::string g4_volume_start,
                 std::string g4_volume_end,
                 std::string g4_prod_process_name,
                 int g4_prod_process_type,
                 int particle_id,
                 double start_time,
                 double end_time,
                 double initial_kin_E,
                 double final_kin_E,
                 double initial_tot_E,
                 double final_tot_E,
                 ROOT::Math::XYZVector initial_mom_direction,
                 ROOT::Math::XYZVector final_mom_direction)
    : start_point_(std::move(start_point)), end_point_(std::move(end_point)), start_g4_vol_name_(std::move(g4_volume_start)),
      end_g4_vol_name_(std::move(g4_volume_end)), origin_g4_process_name_(std::move(g4_prod_process_name)),
      origin_g4_process_type_(g4_prod_process_type), particle_id_(particle_id), global_start_time_(start_time),
      global_end_time_(end_time), initial_kin_E_(initial_kin_E), final_kin_E_(final_kin_E), initial_tot_E_(initial_tot_E),
      final_tot_E_(final_tot_E), initial_mom_direction_(std::move(initial_mom_direction)),
      final_mom_direction_(std::move(final_mom_direction)) {
    setParent(nullptr);
}

ROOT::Math::XYZPoint MCTrack::getStartPoint() const { return start_point_; }

ROOT::Math::XYZPoint MCTrack::getEndPoint() const { return end_point_; }

int MCTrack::getParticleID() const { return particle_id_; }

double MCTrack::getGlobalStartTime() const { return global_start_time_; }

double MCTrack::getGlobalEndTime() const { return global_end_time_; }

int MCTrack::getCreationProcessType() const { return origin_g4_process_type_; }

double MCTrack::getKineticEnergyInitial() const { return initial_kin_E_; }

double MCTrack::getTotalEnergyInitial() const { return initial_tot_E_; }

double MCTrack::getKineticEnergyFinal() const { return final_kin_E_; }

double MCTrack::getTotalEnergyFinal() const { return final_tot_E_; }

ROOT::Math::XYZVector MCTrack::getMomentumDirectionInitial() const { return initial_mom_direction_; }

ROOT::Math::XYZVector MCTrack::getMomentumDirectionFinal() const { return final_mom_direction_; }

std::string MCTrack::getOriginatingVolumeName() const { return start_g4_vol_name_; }

std::string MCTrack::getTerminatingVolumeName() const { return end_g4_vol_name_; }

std::string MCTrack::getCreationProcessName() const { return origin_g4_process_name_; }

/**
 * Object is stored as TRef and can only be accessed if pointed object is in scope
 */
const MCTrack* MCTrack::getParent() const { return parent_.get(); }

void MCTrack::setParent(const MCTrack* mc_track) { parent_ = PointerWrapper<MCTrack>(mc_track); }

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
        << start_g4_vol_name_ << '\n'
        << std::left << std::setw(big_gap) << "Termination in G4Volume: " << std::right << std::setw(small_gap)
        << end_g4_vol_name_ << '\n'
        << std::left << std::setw(big_gap) << "Initial position:" << std::right << std::setw(med_gap) << start_point_.X()
        << " mm | " << std::setw(med_gap) << start_point_.Y() << " mm | " << std::setw(med_gap) << start_point_.Z()
        << " mm\n"
        << std::left << std::setw(big_gap) << "Final position:" << std::right << std::setw(med_gap) << end_point_.X()
        << " mm | " << std::setw(med_gap) << end_point_.Y() << " mm | " << std::setw(med_gap) << end_point_.Z() << " mm\n"
        << std::left << std::setw(big_gap) << "Initial time:" << std::right << std::setw(med_gap) << global_start_time_
        << " ns\n"
        << std::left << std::setw(big_gap) << "Final time:" << std::right << std::setw(med_gap) << global_end_time_
        << " ns\n"
        << std::left << std::setw(big_gap) << "Initial kinetic energy: " << std::right << std::setw(med_gap)
        << initial_kin_E_ << std::setw(small_gap) << " MeV | " << std::left << std::setw(big_gap)
        << "Final kinetic energy: " << std::right << std::setw(med_gap) << final_kin_E_ << std::setw(small_gap)
        << " MeV   \n"
        << std::left << std::setw(big_gap) << "Initial total energy: " << std::right << std::setw(med_gap) << initial_tot_E_
        << std::setw(small_gap) << " MeV | " << std::left << std::setw(big_gap) << "Final total energy: " << std::right
        << std::setw(med_gap) << final_tot_E_ << std::setw(small_gap) << " MeV   \n"
        << std::left << std::setw(big_gap) << "Initial mom. direction:" << std::right << std::setw(med_gap)
        << initial_mom_direction_.X() << " | " << std::setw(med_gap) << initial_mom_direction_.Y() << " | "
        << std::setw(med_gap) << initial_mom_direction_.Z() << "\n"
        << std::left << std::setw(big_gap) << "Final mom. direction:" << std::right << std::setw(med_gap)
        << final_mom_direction_.X() << " | " << std::setw(med_gap) << final_mom_direction_.Y() << " | " << std::setw(med_gap)
        << final_mom_direction_.Z() << "\n";
    if(parent != nullptr) {
        out << "Linked parent: " << parent << '\n';
    } else {
        out << "Linked parent: <nullptr>\n";
    }
    out << std::setfill('-') << std::setw(largest_output) << "" << std::setfill(' ') << '\n';
}

void MCTrack::loadHistory() { parent_.get(); }
void MCTrack::petrifyHistory() { parent_.store(); }
