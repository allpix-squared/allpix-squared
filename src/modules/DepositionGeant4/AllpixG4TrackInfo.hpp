/**
 * @file
 * @brief Defines the handling of the sensitive device
 * @copyright Copyright (c) 2018 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef AllpixG4TrackInfo_H
#define AllpixG4TrackInfo_H 1

#include "G4VUserTrackInformation.hh"

namespace allpix{
    /**
    * @brief Allpix implementation of G4VUserTrackInformation to handle unique track IDs
    */
    class AllpixG4TrackInfo: public G4VUserTrackInformation {
    public:
        /** 
        * @brief Default constructor which automatically assigns new track ID 
        */
        AllpixG4TrackInfo(): G4VUserTrackInformation(), _counter(gCounter++) {};

        /** 
        * @brief Getter for unique ID of track 
        */
        int getID() const {return _counter;};
        
         /** 
        * @brief Static function to reset counter, to be called after every event 
        */
        static void resetCounter() { gCounter = 1; };

    private:
        //Static counter to keep track of assigned track IDs
        static int gCounter;
        //Assigned track ID to track
        int _counter;
    };
}
#endif
