/**
 * @file
 * @brief Implements a G4VUserTrackInformation to carry unique track and parent track IDs
 *
 * @copyright Copyright (c) 2018-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "TrackInfoG4.hpp"
#include "G4VProcess.hh"

using namespace allpix;

TrackInfoG4::TrackInfoG4(int custom_track_id, int parent_track_id, const G4Track* const aTrack)
    : custom_track_id_(custom_track_id), parent_track_id_(parent_track_id),
      particle_id_(aTrack->GetDynamicParticle()->GetPDGcode()), start_time_(aTrack->GetGlobalTime()),
      initial_g4_vol_name_(aTrack->GetVolume()->GetName()), initial_kin_E_(aTrack->GetKineticEnergy()),
      initial_tot_E_(aTrack->GetTotalEnergy()) {
    start_point_ = static_cast<ROOT::Math::XYZPoint>(aTrack->GetPosition());
    const auto* G4Process = aTrack->GetCreatorProcess();
    origin_g4_process_type_ = (G4Process != nullptr) ? G4Process->GetProcessSubType() : -1;
    origin_g4_process_name_ = (G4Process != nullptr) ? static_cast<std::string>(G4Process->GetProcessName()) : "none";
}

void TrackInfoG4::finalizeInfo(const G4Track* const aTrack) {
    final_g4_vol_name_ = aTrack->GetVolume()->GetName();
    final_kin_E_ = aTrack->GetKineticEnergy();
    final_tot_E_ = aTrack->GetTotalEnergy();
    end_point_ = static_cast<ROOT::Math::XYZPoint>(aTrack->GetPosition());
    end_time_ = aTrack->GetGlobalTime();
}
