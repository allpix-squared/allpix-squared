/**
 * @file
 * @brief The TrackInfoManager class, contains a factory method for TrackInfoG4 to be used in AP2 as well as handling and
 * dispatching MCTracks
 *
 * @copyright Copyright (c) 2018-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef TrackInfoManager_H
#define TrackInfoManager_H 1

#include <map>

#include "G4Track.hh"
#include "TrackInfoG4.hpp"

#include "core/module/Event.hpp"
#include "core/module/Module.hpp"
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
        explicit TrackInfoManager(bool record_all);

        /**
         * @brief Factory method for TrackInfoG4 instances
         * @param track A pointer to the G4Track for which the TrackInfoG4 is used
         * @return unique_ptr holding the TrackInfoG4
         *
         * This factory method will take care that it will only assign every track id once for this
         * TrackInfoManager instance, until reset @see #resetTrackInfoManager
         */
        std::unique_ptr<TrackInfoG4> makeTrackInfo(const G4Track* const track);

        /**
         * @brief Will take a MCTrack and attempt to store it
         * @param the_track_info The MCTrack to be (possibly) stored
         *
         * It will be stored if it was registered to be stored (@see #setTrackInfoToBeStored), otherwise deleted
         */
        void storeTrackInfo(std::unique_ptr<TrackInfoG4> the_track_info);

        /**
         * @brief Will register a track id to be stored
         * @param track_id The id of the track to be stored
         *
         * The track itself will have to be provided via @see storeTrackInfo once finished
         */
        void setTrackInfoToBeStored(int track_id);

        /**
         * @brief Reset of the TrackInfoManager instance
         *
         * This will reset the track id counter, the tracks which are registered to be stored and the already stored tracks,
         * regardless if they have been dispatched or not. Make sure to call \ref dispatchMessage before if the tracks
         * should be dispatched
         */
        void resetTrackInfoManager();

        /**
         * @brief Dispatch the stored tracks as a MCTrackMessage
         * @param module The module which is responsible for dispatching the message
         * @param messenger The messenger used to dispatch it
         * @param event The event to dispatch the message to
         */
        void dispatchMessage(Module* module, Messenger* messenger, Event* event);

        /**
         * @brief Populate the #stored_tracks_ with MCTrack objects
         * @warning Must only be called once Geant4 finished stepping through all the G4Track objects
         * guaranteed
         */
        void createMCTracks();

        /**
         * @brief Returns a pointer to the MCTrack object in the #stored_tracks_ or a nullptr if not found
         * @param track_id The id of the track for which to retrieve the pointer
         * @return Const pointer to the MCTrack object or a nullptr if track_id is not found
         * @warning Results are invalidated by any reallocation iof the internal #stored_tracks_ vector
         */
        MCTrack const* findMCTrack(int track_id) const;

    private:
        /**
         * @brief Will internally set all the parent-child relations between stored tracks
         * @warning This must only be called once all the tracks are created (@see #createMCTracks) and no reallocation of
         * the back-end vector is guaranteed
         */
        void set_all_track_parents();

        // Counter to store highest assigned track id
        int counter_{};

        // Store configuration whether all tracks or only those connected to sensor should be stored
        bool record_all_{};

        // Geant4 id to custom id translation
        std::map<int, int> g4_to_custom_id_;
        // Custom id to custom parent id tracking
        std::map<int, int> track_id_to_parent_id_;
        // List of track ids to be stored if they are provided via #storeTrackInfo
        std::vector<int> to_store_track_ids_;
        // The TrackInfoG4 instances which are handed over to this track manager
        std::vector<std::unique_ptr<TrackInfoG4>> stored_track_infos_;
        // The MCTrack vector which is dispatched via #dispatchMessage
        std::vector<MCTrack> stored_tracks_;
        // Ids ins same order as tracks stored in #stored_tracks_
        std::vector<int> stored_track_ids_;
        // Id to index in #stored_tracks_ for easier handling
        std::map<int, MCTrack const*> id_to_track_;
    };
} // namespace allpix
#endif /* TrackInfoManager_H */
