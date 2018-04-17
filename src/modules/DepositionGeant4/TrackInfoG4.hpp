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
         * @brief Only available constructor
         * @param custom_track_id The custom id for this track
         * @param parent_track_id The custom id of parent track
         * @param G4TrackID The Geant4 id of the track to which the info is attached
         */
        TrackInfoG4(int custom_track_id, int parent_track_id, const G4Track* const aTrack);

        /**
         * @brief Default constructor deleted
         */
        TrackInfoG4() = delete;

        /**
         * @brief Getter for custom id of track
         * @return The custom track id
         */
        int getID() const { return custom_track_id_; };

        /**
         * @brief Getter for parent's track id of track
         * @return The parent's custrom track id
         */
        int getParentID() const { return parent_track_id_; };

        /**
         * @brief Get the TrackInfoG4 user info and update it with the information from the G4Track*
         * @param aTrack A pointer to a G4Track instance which represents this track's final state
         */
        void finaliseInfo(const G4Track* const aTrack);

        /**
         * @brief Release the MCTrack owned by this TrackInfoG4 to pass on ownership
         * @return The MCTrack object encapsulated in a unique_ptr
         */
        std::unique_ptr<MCTrack> releaseMCTrack();

    private:
        // Assigned track id to track
        int custom_track_id_;
        // Parent's track id
        int parent_track_id_;
        // Owning a MCTrack instance
        std::unique_ptr<MCTrack> _mcTrack;
    };
} // namespace allpix
#endif
