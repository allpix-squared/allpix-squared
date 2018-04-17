/**
 * @file
 * @brief The TrackInfoManager class, contains a factory method for TrackInfoG4 to be used in AP2 as well as handling and
 * dispatching MCTracks
 * @copyright Copyright (c) 2018 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef TrackInfoManager_H
#define TrackInfoManager_H 1

#include <map>

#include "G4Track.hh"
#include "TrackInfoG4.hpp"

#include "core/messenger/Messenger.hpp"
#include "objects/MCTrack.hpp"

namespace allpix {
    /**
     * @brief The TrackInfoManager is a factory for TrackInfoG4 objects and manages MCTracks within AP2
     */
    class TrackInfoManager {
    public:
        /**
         * @brief Default constructor
         */
        TrackInfoManager();

        /**
         * @brief Factory method for TrackInfoG4 instances
         * @param aTrack A pointer to the G4Track for which the TrackInfoG4 is used
         * @return unique_ptr holding the TrackInfoG4
         *
         * This factory method will take care that it will only assign every track id once for this
         * TrackInfoManager instance, until reset @see #resetTrackInfoManager
         */
        std::unique_ptr<TrackInfoG4> makeTrackInfo(const G4Track* const aTrack);

        /**
         * @brief Will take a MCTrack and attempt to store it
         * @param theTrack The MCTrack to be (possibly) stored
         *
         * It will be stored if it was registered to be stored (@see #setTrackInfoToBeStored), otherwise deleted
         */
        void storeTrackInfo(std::unique_ptr<MCTrack> theTrack);

        /**
         * @brief Will register a track id to be stored
         * @param theTrack The id of teh track to be stored
         *
         * The track itself will have to be provided via @see storeTrackInfo once finished
         */
        void setTrackInfoToBeStored(int trackID);

        /**
         * @brief Reset of the TrackInfoManager instance
         *
         * This will reset the track id counter, the tracks which are registered to be stored and the already stored tracks,
         * regardless if they have been dispatched or not. Make sure to call @see #dispatchMesseges before if the tracks
         * should be dispatched
         */
        void resetTrackInfoManager();

        /**
         * @brief Dispatch the stored tracks as a MCTrackMessage
         * @param module The module which is responsible for dispatching the message
         * @param messenger The messenger used to dispatch it
         */
        void dispatchMesseges(Module* module, Messenger* messenger);

    private:
        int counter_;
        std::map<int, int> g4_to_custom_id_;
        std::map<int, std::unique_ptr<MCTrack>> to_store_tracks_;
    };
} // namespace allpix
#endif
