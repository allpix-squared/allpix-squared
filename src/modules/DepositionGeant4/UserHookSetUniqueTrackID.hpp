/**
 * @file
 * @brief Defines the handling of the sensitive device
 * @copyright Copyright (c) 2018 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef UserHookSetUniqueTrackID_H
#define UserHookSetUniqueTrackID_H 1

#include "G4UserTrackingAction.hh"
#include "G4Track.hh"

namespace allpix {
    /**
     * @brief Assigns every G4Track a AllpixG4TrackInfo which carries the unique track ID 
     */
    class UserHookSetUniqueTrackID : public G4UserTrackingAction {
    public:
    /**
     * @brief Default constructor 
     */
    UserHookSetUniqueTrackID() = default;

    /**
     * @brief Default destructor 
     */
    ~UserHookSetUniqueTrackID() = default;

    /**
     * @brief Called for every G4Track at beginning 
     */
    void PreUserTrackingAction(const G4Track* aTrack);
};

}
#endif
