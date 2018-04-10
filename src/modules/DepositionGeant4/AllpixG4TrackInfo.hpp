/**
 * @file
 * @brief Defines a concrete implementation of a G4VUserTrackInformation to carry unique track and parent track IDs
 * @copyright Copyright (c) 2018 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef AllpixG4TrackInfo_H
#define AllpixG4TrackInfo_H 1

#include <map>
#include "G4Track.hh"
#include "G4VUserTrackInformation.hh"

namespace allpix {
    /**
     * @brief Allpix implementation of G4VUserTrackInformation to handle unique track IDs
     */
    class AllpixG4TrackInfo : public G4VUserTrackInformation {
    public:
        /**
         * @brief Default constructor which automatically assigns new track ID
         * @param G4TrackID The G4 ID of the track to which the info is attached
         */
        AllpixG4TrackInfo(const G4Track* const aTrack);

        /**
         * @brief Getter for unique ID of track
         */
        int getID() const { return _customTrackID; };

        /**
         * @brief Getter for parent's track ID of track
         */
        int getParentID() const { return _parentTrackID; };

        /**
         * @brief Static function to reset the state, i.e. counters etc. To be called after every event
         */
        static void reset();

    private:
        // Static counter to keep track of assigned track IDs
        static int gCounter;
        // Static map to link G4 ID to custom ID
        static std::map<int, int> gG4ToCustomID;
        // Assigned track ID to track
        int _customTrackID;
        // Parent's track ID
        int _parentTrackID;
    };
} // namespace allpix
#endif
