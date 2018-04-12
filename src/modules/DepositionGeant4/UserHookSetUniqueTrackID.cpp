#include "UserHookSetUniqueTrackID.hpp"
#include "TrackInfoG4.hpp"

using namespace allpix;

void UserHookSetUniqueTrackID::PreUserTrackingAction(const G4Track* aTrack) {
    auto theTrack = const_cast<G4Track*>(aTrack); // NOLINT
    theTrack->SetUserInformation(new TrackInfoG4(aTrack));
}

void UserHookSetUniqueTrackID::PostUserTrackingAction(const G4Track* aTrack) {
    auto userInfo = dynamic_cast<TrackInfoG4*>(aTrack->GetUserInformation());
    userInfo->finaliseInfo(aTrack);
}
