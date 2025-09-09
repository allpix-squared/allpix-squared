/**
 * @file
 * @brief Implementation of the TrackInfoManager class
 *
 * @copyright Copyright (c) 2018-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "TrackInfoManager.hpp"

using namespace allpix;

TrackInfoManager::TrackInfoManager(bool record_all) : counter_(1), record_all_(record_all) {}

std::unique_ptr<TrackInfoG4> TrackInfoManager::makeTrackInfo(const G4Track* const track) {
    auto custom_id = counter_++;
    auto G4ParentID = track->GetParentID();
    auto parent_track_id = G4ParentID == 0 ? G4ParentID : g4_to_custom_id_.at(G4ParentID);
    g4_to_custom_id_[track->GetTrackID()] = custom_id;
    track_id_to_parent_id_[custom_id] = parent_track_id;
    return std::make_unique<TrackInfoG4>(custom_id, parent_track_id, track);
}

void TrackInfoManager::setTrackInfoToBeStored(int track_id) {
    auto element = std::find(to_store_track_ids_.begin(), to_store_track_ids_.end(), track_id);
    // If track id is not present we add it, otherwise skip as we only need each track once
    if(element == to_store_track_ids_.end()) {
        to_store_track_ids_.emplace_back(track_id);
    }
}

void TrackInfoManager::storeTrackInfo(std::unique_ptr<TrackInfoG4> the_track_info) {
    auto track_id = the_track_info->getID();
    auto element = std::find(to_store_track_ids_.begin(), to_store_track_ids_.end(), track_id);

    if(record_all_ || element != to_store_track_ids_.end()) {
        LOG(DEBUG) << "Storing MCTrack with ID " << track_id;
        stored_track_infos_.push_back(std::move(the_track_info));
    } else {
        LOG(DEBUG) << "Not storing MCTrack with ID " << track_id;
    }

    if(element != to_store_track_ids_.end()) {
        to_store_track_ids_.erase(element);
    }
}

void TrackInfoManager::resetTrackInfoManager() {
    counter_ = 1;
    stored_tracks_.clear();
    to_store_track_ids_.clear();
    g4_to_custom_id_.clear();
    track_id_to_parent_id_.clear();
    stored_track_infos_.clear();
    stored_track_ids_.clear();
    id_to_track_.clear();
}

void TrackInfoManager::dispatchMessage(Module* module, Messenger* messenger, Event* event) {
    set_all_track_parents();
    IFLOG(DEBUG) {
        LOG(DEBUG) << "Dispatching " << stored_tracks_.size() << " MCTrack(s) from TrackInfoManager::dispatchMessage()";
        for(auto const& mc_track : stored_tracks_) {
            LOG(DEBUG) << "MCTrack originates at: " << Units::display(mc_track.getStartPoint(), {"mm", "um"})
                       << " and terminates at: " << Units::display(mc_track.getEndPoint(), {"mm", "um"});
        }
    }
    auto mc_track_message = std::make_shared<MCTrackMessage>(std::move(stored_tracks_));
    messenger->dispatchMessage(module, std::move(mc_track_message), event);
}

MCTrack const* TrackInfoManager::findMCTrack(int track_id) const {
    auto it = id_to_track_.find(track_id);
    return (it == id_to_track_.end()) ? nullptr : it->second;
}

void TrackInfoManager::createMCTracks() {
    // Reserve size so we don't move the vector around and change addresses:
    stored_tracks_.reserve(stored_track_infos_.size());

    for(auto& track_info : stored_track_infos_) {
        stored_tracks_.emplace_back(track_info->getStartPoint(),
                                    track_info->getEndPoint(),
                                    track_info->getOriginatingVolumeName(),
                                    track_info->getTerminatingVolumeName(),
                                    track_info->getCreationProcessName(),
                                    track_info->getCreationProcessType(),
                                    track_info->getParticleID(),
                                    track_info->getStartTime(),
                                    track_info->getEndTime(),
                                    track_info->getKineticEnergyInitial(),
                                    track_info->getKineticEnergyFinal(),
                                    track_info->getTotalEnergyInitial(),
                                    track_info->getTotalEnergyFinal(),
                                    track_info->getMomentumDirectionInitial(),
                                    track_info->getMomentumDirectionFinal());

        id_to_track_[track_info->getID()] = &stored_tracks_.back();
        stored_track_ids_.emplace_back(track_info->getID());
    }
}

void TrackInfoManager::set_all_track_parents() {
    for(size_t ix = 0; ix < stored_track_ids_.size(); ++ix) {
        auto track_id = stored_track_ids_[ix];
        auto parent_id = track_id_to_parent_id_[track_id];
        stored_tracks_[ix].setParent(findMCTrack(parent_id));
    }
}
