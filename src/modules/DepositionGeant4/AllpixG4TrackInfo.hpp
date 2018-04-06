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
#include <map>

namespace allpix{
    /**
    * @brief Allpix implementation of G4VUserTrackInformation to handle unique track IDs
    */
    class AllpixG4TrackInfo: public G4VUserTrackInformation {
    public:
        /** 
        * @brief Default constructor which automatically assigns new track ID
        * @param G4TrackID The G4 ID of the track to which the info is attached 
        */
        AllpixG4TrackInfo(int G4TrackID);

        /** 
        * @brief Getter for unique ID of track 
        */
        int getID() const {return _counter;};
        
        /** 
        * @brief Static function to reset counter, to be called after every event 
        */
        static void resetCounter();

        /** 
        * @brief Static function to translate G4ID to custom ID
        * @param G4ID The G4 ID to be translated 
        */
        static int getCustomIDfromG4ID(int G4ID);
    private:
        //Static counter to keep track of assigned track IDs
        static int gCounter;
        //Static map to link G4 ID to custom ID
        static std::map<int, int> gG4ToCustomID;
        //Assigned track ID to track
        int _counter;
    };
}
#endif
