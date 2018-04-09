#include "UserHookSetUniqueTrackID.hpp"
#include "AllpixG4TrackInfo.hpp"

using namespace allpix;

void UserHookSetUniqueTrackID::PreUserTrackingAction(const G4Track* aTrack) {
    auto theTrack = const_cast<G4Track*>(aTrack); // NOLINT
    theTrack->SetUserInformation(new AllpixG4TrackInfo(aTrack->GetTrackID()));
}
