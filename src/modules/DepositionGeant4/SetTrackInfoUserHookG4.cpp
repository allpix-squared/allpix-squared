#include "SetTrackInfoUserHookG4.hpp"
#include "TrackInfoG4.hpp"

using namespace allpix;

void SetTrackInfoUserHookG4::PreUserTrackingAction(const G4Track* aTrack) {
    auto theTrack = const_cast<G4Track*>(aTrack); // NOLINT
    auto particle = aTrack->GetDefinition();

    // Check if this is an ion (charge larger than an alpha particle) and kill if it is not the primary ion:
    if((particle->GetPDGCharge() > 2. + std::numeric_limits<double>::epsilon()) && aTrack->GetTrackID() > 1) {
        theTrack->SetTrackStatus(fStopAndKill);
    }

    if(aTrack->GetUserInformation() == nullptr) {
        auto trackInfo = track_info_mgr_ptr_->makeTrackInfo(aTrack);
        // Release ownership of the TrackInfoG4 instance
        theTrack->SetUserInformation(trackInfo.release());
    }
}

void SetTrackInfoUserHookG4::PostUserTrackingAction(const G4Track* aTrack) {
    auto userInfo = dynamic_cast<TrackInfoG4*>(aTrack->GetUserInformation());
    userInfo->finalizeInfo(aTrack);
    // Regain ownership of the TrackInfoG4, and remove it from the G4Track
    auto userInfoOwningPtr = std::unique_ptr<TrackInfoG4>(userInfo);
    auto theTrack = const_cast<G4Track*>(aTrack); // NOLINT
    theTrack->SetUserInformation(nullptr);
    track_info_mgr_ptr_->storeTrackInfo(std::move(userInfoOwningPtr));
}
