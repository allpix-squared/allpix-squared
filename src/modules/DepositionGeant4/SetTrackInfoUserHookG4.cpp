#include "SetTrackInfoUserHookG4.hpp"
#include "TrackInfoG4.hpp"

using namespace allpix;

void SetTrackInfoUserHookG4::PreUserTrackingAction(const G4Track* aTrack) {
    auto theTrack = const_cast<G4Track*>(aTrack); // NOLINT
    auto particle = aTrack->GetDefinition();
    auto particle_lifetime = particle->GetPDGLifeTime();

    // Unstable particles which are not the primary particle and have a lifetime longer than the lifetime_cut
    // should be killed to stop the decay chain:
    if(particle_lifetime > decay_cutoff_time_ && aTrack->GetTrackID() > 1) {

        // Only give the warning once to prevent too many errors given per event
        LOG_ONCE(WARNING) << "The track of " << particle->GetParticleName() << ", with a lifetime of " << particle_lifetime
                          << "ns, will not be propagated for this simulation because its lifetime is too long!\n"
                          << "If you do want to propagate this particle, set the decay_cutoff_time to a value larger than "
                          << particle_lifetime << "ns.";
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
