#include "TrackInfoG4.hpp"

using namespace allpix;

// Initialisation of static members for TrackInfoG4 which are needed to store state
int TrackInfoG4::gCounter = 1;
std::map<int, int> TrackInfoG4::gG4ToCustomID = std::map<int, int>();

TrackInfoG4::TrackInfoG4(const G4Track* const aTrack) : custom_track_id_(gCounter++) {
    gG4ToCustomID[aTrack->GetTrackID()] = custom_track_id_;
    // If the Geant4 ID is 0 we mustn't convert
    auto G4ParentID = aTrack->GetParentID();
    parent_track_id_ = G4ParentID == 0 ? G4ParentID : gG4ToCustomID.at(G4ParentID);
}

void TrackInfoG4::reset() {
    gCounter = 1;
    gG4ToCustomID.clear();
}
