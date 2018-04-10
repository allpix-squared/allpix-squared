#include "AllpixG4TrackInfo.hpp"

using namespace allpix;

int AllpixG4TrackInfo::gCounter = 1;
std::map<int, int> AllpixG4TrackInfo::gG4ToCustomID = std::map<int, int>();

AllpixG4TrackInfo::AllpixG4TrackInfo(const G4Track* const aTrack) : _customTrackID(gCounter++) {
    gG4ToCustomID[aTrack->GetTrackID()] = _customTrackID;
    // If the Geant4 ID is 0 we mustn't convert
    auto G4ParentID = aTrack->GetParentID();
    _parentTrackID = G4ParentID == 0 ? G4ParentID : gG4ToCustomID.at(G4ParentID);
}

void AllpixG4TrackInfo::reset() {
    gCounter = 1;
    gG4ToCustomID.clear();
}
