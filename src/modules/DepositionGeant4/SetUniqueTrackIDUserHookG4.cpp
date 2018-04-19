#include "SetUniqueTrackIDUserHookG4.hpp"
#include "TrackInfoG4.hpp"

using namespace allpix;

void SetUniqueTrackIDUserHookG4::PreUserTrackingAction(const G4Track* aTrack) {
    auto theTrack = const_cast<G4Track*>(aTrack); // NOLINT
    if(aTrack->GetUserInformation() == nullptr) {
        auto trackInfo = track_info_mgr_ptr_->makeTrackInfo(aTrack);
        theTrack->SetUserInformation(trackInfo.release());
    }
}

void SetUniqueTrackIDUserHookG4::PostUserTrackingAction(const G4Track* aTrack) {
    auto userInfo = dynamic_cast<TrackInfoG4*>(aTrack->GetUserInformation());
    userInfo->finaliseInfo(aTrack);
    auto userInfoOwningPtr = std::unique_ptr<TrackInfoG4>(userInfo);
    auto theTrack = const_cast<G4Track*>(aTrack); // NOLINT
    theTrack->SetUserInformation(nullptr);
    track_info_mgr_ptr_->storeTrackInfo(std::move(userInfoOwningPtr));
}
