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

MCTrack::MCTrack(
    ROOT::Math::XYZPoint start_point, std::string G4Volume, std::string Test, int particle_id, int trackID, int parentID)
    : start_point_(std::move(start_point)), particle_id_(particle_id), trackID_(trackID) {
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
