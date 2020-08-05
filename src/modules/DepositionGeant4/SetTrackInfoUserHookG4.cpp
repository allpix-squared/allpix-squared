/**
 * @file
 * @brief Implements a user hook for Geant4 to assign custom track information via TrackInfoG4 objects
 *
 * @copyright Copyright (c) 2018-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "SetTrackInfoUserHookG4.hpp"
#include "DepositionGeant4Module.hpp"
#include "TrackInfoG4.hpp"

using namespace allpix;

void SetTrackInfoUserHookG4::PreUserTrackingAction(const G4Track* aTrack) {
    auto theTrack = const_cast<G4Track*>(aTrack); // NOLINT
    auto particle = aTrack->GetDefinition();
    auto particle_lifetime = particle->GetPDGLifeTime();

    /**
     * Unstable particles which are not the primary particle and have a lifetime longer than the lifetime_cut
     * should be killed to stop the decay chain
     * Note: Stable particles have a lifetime of either -1 or 0. Geant4 has exited states which also have a lifetime of 0.
     *       The ">" sign was chosen to prevent stable particles or instantly decaying exited states
     *       from being killed if the decay_cutoff_time would be set to 0.
     */
    if(particle_lifetime > module_->decay_cutoff_time_ && aTrack->GetTrackID() > 1) {

        // Only give the warning once to prevent too many errors given per event
        LOG_ONCE(WARNING)
            << "The track of " << particle->GetParticleName() << ", with a lifetime of "
            << Units::display(particle_lifetime, {"us", "ns"})
            << "ns, will not be propagated for this simulation because its lifetime is too long!" << std::endl
            << "If you do want to propagate this particle, set the decay_cutoff_time to a value larger than its lifetime.";
        theTrack->SetTrackStatus(fStopAndKill);
    }

    if(aTrack->GetUserInformation() == nullptr) {
        auto trackInfo = module_->track_info_manager_->makeTrackInfo(aTrack);
        // Release ownership of the TrackInfoG4 instance
        theTrack->SetUserInformation(trackInfo.release());
    }
}

void SetTrackInfoUserHookG4::PostUserTrackingAction(const G4Track* aTrack) {
    auto userInfo = static_cast<TrackInfoG4*>(aTrack->GetUserInformation());
    userInfo->finalizeInfo(aTrack);
    // Regain ownership of the TrackInfoG4, and remove it from the G4Track
    auto userInfoOwningPtr = std::unique_ptr<TrackInfoG4>(userInfo);
    auto theTrack = const_cast<G4Track*>(aTrack); // NOLINT
    theTrack->SetUserInformation(nullptr);
    module_->track_info_manager_->storeTrackInfo(std::move(userInfoOwningPtr));
}
