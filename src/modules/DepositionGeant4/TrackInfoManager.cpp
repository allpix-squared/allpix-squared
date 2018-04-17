/**
 * @file
 * @brief Implementation of the TrackInfoManager class
 * @copyright Copyright (c) 2018 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "TrackInfoManager.hpp"

using namespace allpix;

TrackInfoManager::TrackInfoManager() : counter_(1) {}

std::unique_ptr<TrackInfoG4> TrackInfoManager::makeTrackInfo(const G4Track* const aTrack) {
    auto i = counter_++;
    auto G4ParentID = aTrack->GetParentID();
    auto parent_track_id = G4ParentID == 0 ? G4ParentID : g4_to_custom_id_.at(G4ParentID);
    g4_to_custom_id_[aTrack->GetTrackID()] = i;
    return std::unique_ptr<TrackInfoG4>(new TrackInfoG4(i, parent_track_id, aTrack));
}

void TrackInfoManager::setTrackInfoToBeStored(int trackID) {
    to_store_tracks_[trackID] = nullptr;
}

void TrackInfoManager::storeTrackInfo(std::unique_ptr<MCTrack> theTrack) {
    auto ID = theTrack->getTrackID();
    auto element = to_store_tracks_.find(ID);
    if(element != to_store_tracks_.end()) {
        (*element).second = std::move(theTrack);
    }
}

void TrackInfoManager::resetTrackInfoManager() {
    counter_ = 1;
    to_store_tracks_.clear();
    g4_to_custom_id_.clear();
}

void TrackInfoManager::dispatchMesseges(Module* module, Messenger* messenger) {
    auto test = std::vector<MCTrack>();
    for(auto it = to_store_tracks_.begin(); it != to_store_tracks_.end(); ++it) {
        if((*it).second)
            test.push_back(*std::move((*it).second));
    }
    auto mc_track_message = std::make_shared<MCTrackMessage>(std::move(test));
    messenger->dispatchMessage(module, mc_track_message);
}
