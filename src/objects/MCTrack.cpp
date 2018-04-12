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
                 std::string G4Volume,
                 std::string prodProcessName,
                 int prodProcessType,
                 int particle_id,
                 int trackID,
                 int parentID,
                 double initialEnergy)
    : start_point_(std::move(start_point)), particle_id_(particle_id), trackID_(trackID), parentID_(parentID),
      initialEnergy_(initialEnergy), originG4ProcessType_(prodProcessType), originG4VolName_(G4Volume),
      originG4ProcessName_(prodProcessName) {
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
    return trackID_;
}

namespace allpix {
    std::ostream& operator<<(std::ostream& stream, const MCTrack& track) {
        stream << "\n ------- Printing track information for track: " << track.trackID_ << " (" << &track << ") -------\n"
               << "Particle type (PDG ID): " << track.particle_id_ << " | Parent track ID: " << track.parentID_ << '\n'
               << "Production process: " << track.originG4ProcessName_ << " (G4 process type: " << track.originG4ProcessType_
               << ")\n"
               << "Production in G4Volume: " << track.originG4VolName_ << '\n'
               << "Initial position:\t " << track.start_point_.X() << " mm\t|" << track.start_point_.Y() << " mm\t|"
               << track.start_point_.Z() << " mm\n"
               << "Final position:\t\t " << track.end_point_.X() << " mm\t|" << track.end_point_.Y() << " mm\t|"
               << track.end_point_.Z() << " mm\n"
               << "Initial energy: " << track.initialEnergy_ << " MeV \t Final energy: " << track.finalEnergy_ << " MeV\n"
               << "---------------------------------------------------------------\n";
        return stream;
    }
} // namespace allpix

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
