/**
 * @file
 * @brief Defines a user hook for Geant4 to assign custom track IDs which are unique
 * @copyright Copyright (c) 2018 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef SetUniqueTrackIDUserHookG4_H
#define SetUniqueTrackIDUserHookG4_H 1

#include "G4Track.hh"
#include "G4UserTrackingAction.hh"

namespace allpix {
    /**
     * @brief Assigns every G4Track a TrackInfoG4 which carries the unique track ID
     */
    class SetUniqueTrackIDUserHookG4 : public G4UserTrackingAction {
    public:
        /**
         * @brief Default constructor
         */
        SetUniqueTrackIDUserHookG4() = default;

        /**
         * @brief Default destructor
         */
        ~SetUniqueTrackIDUserHookG4() = default;

        /**
         * @brief Called for every G4Track at beginning
         */
        void PreUserTrackingAction(const G4Track* aTrack);

        /**
         * @brief Called for every G4Track at end
         */
        void PostUserTrackingAction(const G4Track* aTrack);
    };

} // namespace allpix
#endif
