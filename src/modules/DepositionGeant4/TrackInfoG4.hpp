/**
 * @file
 * @brief Defines a concrete implementation of a G4VUserTrackInformation to carry unique track and parent track IDs
 * @copyright Copyright (c) 2018 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef TrackInfoG4_H
#define TrackInfoG4_H 1

#include <map>

#include "G4Track.hh"
#include "G4VUserTrackInformation.hh"

#include "objects/MCTrack.hpp"

#include "core/messenger/Messenger.hpp"

namespace allpix {
    /**
     * @brief Allpix implementation of G4VUserTrackInformation to handle unique track IDs
     */
    class TrackInfoG4 : public G4VUserTrackInformation {
    public:
        /**
         * @brief Default constructor which automatically assigns new track ID
         * @param G4TrackID The G4 ID of the track to which the info is attached
         */
        TrackInfoG4(const G4Track* const aTrack);

        /**
         * @brief Getter for unique ID of track
         */
        int getID() const { return custom_track_id_; };

        /**
         * @brief Getter for parent's track ID of track
         */
        int getParentID() const { return parent_track_id_; };

        /**
         * @brief Static function to reset the state, i.e. counters etc. To be called after every event
         */
        static void reset(Module* module, Messenger* messenger);
        static void storeTrack(std::unique_ptr<MCTrack> theTrack);
        static void registerTrack(int trackID);

        void finaliseInfo(const G4Track* const aTrack);

    private:
        // Static counter to keep track of assigned track IDs
        static int gCounter;
        // Static map to link G4 ID to custom ID
        static std::map<int, int> gG4ToCustomID;
        static std::map<int, std::unique_ptr<MCTrack>> gStoredTracks;

        std::unique_ptr<MCTrack> _mcTrack;

        // Assigned track ID to track
        int custom_track_id_;
        // Parent's track ID
        int parent_track_id_;
    };
} // namespace allpix
#endif
