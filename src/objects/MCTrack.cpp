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
                 int parent_id,
                 double initial_kin_E,
                 double initial_tot_E)
    : start_point_(std::move(start_point)), particle_id_(particle_id), track_id_(track_id), parent_id_(parent_id),
      initial_kin_E_(initial_kin_E), initial_tot_E_(initial_tot_E), origin_g4_process_type_(g4_prod_process_type),
      origin_g4_vol_name_(g4_volume), origin_g4_process_name_(g4_prod_process_name) {
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

int MCTrack::getParentTrackID() const {
    return parent_id_;
}
/**
 * Object is stored as TRef and can only be accessed if pointed object is in scope
 */
const MCTrack* MCTrack::getParent() const {
    return dynamic_cast<MCTrack*>(parent_.GetObject());
}

void MCTrack::setParent(const MCTrack* mc_track) {
    parent_ = const_cast<MCTrack*>(mc_track); // NOLINT
}

ClassImp(MCTrack)

    namespace allpix {
    std::ostream& operator<<(std::ostream& stream, const MCTrack& track) {
        stream << "\n ------- Printing track information for track: " << track.track_id_ << " (" << &track << ") -------\n"
               << "Particle type (PDG ID): " << track.particle_id_ << " | Parent track ID: " << track.parent_id_ << '\n'
               << "Production process: " << track.origin_g4_process_name_
               << " (G4 process type: " << track.origin_g4_process_type_ << ")\n"
               << "Production in G4Volume: " << track.origin_g4_vol_name_ << '\n'
               << "Initial position:\t " << track.start_point_.X() << " mm\t|" << track.start_point_.Y() << " mm\t|"
               << track.start_point_.Z() << " mm\n"
               << "Final position:\t\t " << track.end_point_.X() << " mm\t|" << track.end_point_.Y() << " mm\t|"
               << track.end_point_.Z() << " mm\n"
               << "Initial kinetic energy: " << track.initial_kin_E_
               << " MeV \t Final kinetic energy: " << track.final_kin_E_ << " MeV\n"
               << "Initial total energy: " << track.initial_tot_E_ << " MeV \t Final total energy: " << track.final_tot_E_
               << " MeV\n"
               << "Parent track: " << track.parent_.GetObject() << '\n'
               << " -----------------------------------------------------------------\n";
        return stream;
    }
} // namespace allpix
