/**
 * @file
 * @brief Implements a user hook for Geant4 to assign custom track information via TrackInfoG4 objects
 *
 * @copyright Copyright (c) 2018-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "SetTrackInfoUserHookG4.hpp"

#include <memory>
#include <utility>

#include <G4Track.hh>

#include "DepositionGeant4Module.hpp"
#include "TrackInfoG4.hpp"

using namespace allpix;

void SetTrackInfoUserHookG4::PreUserTrackingAction(const G4Track* aTrack) {
    auto theTrack = const_cast<G4Track*>(aTrack); // NOLINT
    if(aTrack->GetUserInformation() == nullptr) {
        auto trackInfo = DepositionGeant4Module::track_info_manager_->makeTrackInfo(aTrack);
        // Release ownership of the TrackInfoG4 instance
        theTrack->SetUserInformation(trackInfo.release());
    }
}

void SetTrackInfoUserHookG4::PostUserTrackingAction(const G4Track* aTrack) {
    auto* userInfo = dynamic_cast<TrackInfoG4*>(aTrack->GetUserInformation());
    userInfo->finalizeInfo(aTrack);
    // Regain ownership of the TrackInfoG4, and remove it from the G4Track
    auto userInfoOwningPtr = std::unique_ptr<TrackInfoG4>(userInfo);
    auto theTrack = const_cast<G4Track*>(aTrack); // NOLINT
    theTrack->SetUserInformation(nullptr);
    DepositionGeant4Module::track_info_manager_->storeTrackInfo(std::move(userInfoOwningPtr));
}
