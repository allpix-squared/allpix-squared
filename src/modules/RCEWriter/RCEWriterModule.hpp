/**
 * @file
 * @brief Definition of RCE Writer Module
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <map>
#include <string>

#include <TFile.h>
#include <TTree.h>

#include "core/config/Configuration.hpp"

#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"

#include "core/module/Module.hpp"
#include "objects/PixelHit.hpp"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to write object data to ROOT trees in RCE format for telescope reconstruction
     *
     * Listens to the PixelHitMessage. On initialization, creates an Event tree and it's respective branches, and
     * a root sub-directory for each detector with the name Plane* where * is the detector number. In each detector
     * sub-directory, it creates a Hits tree. Upon receiving the Pixel Hit messages, it writes the information to
     * the respective trees.
     */
    class RCEWriterModule : public Module {
    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_mgr Pointer to the geometry manager, containing the detectors
         */
        RCEWriterModule(Configuration& config, Messenger*, GeometryManager*);
        /**
         * @brief Destructor deletes the internal objects used to build the ROOT Tree
         */
        ~RCEWriterModule() override = default;

        /**
         * @brief Opens the file to write the objects to, and initializes the trees
         */
        void init() override;

        /**
         * @brief Writes the objects fetched to their specific tree
         */
        void run(unsigned int) override;

        /**
         * @brief Write the output file
         */
        void finalize() override;

    private:
        GeometryManager* geo_mgr_;

        // Struct to store tree and information for each detector
        struct sensor_data {
            static constexpr int kMaxHits = (1 << 14);
            TTree* tree; // no unique_ptr, ROOT takes ownership
            Int_t nhits_;
            Int_t pix_x_[kMaxHits];
            Int_t pix_y_[kMaxHits];
            Int_t value_[kMaxHits];
            Int_t timing_[kMaxHits];
            Int_t hit_in_cluster_[kMaxHits];
        };
        // The map from detector names to the respective sensor_data struct
        std::map<std::string, sensor_data> sensors_;

        // Relevant information for the Event tree
        ULong64_t timestamp_;
        ULong64_t frame_number_;
        ULong64_t trigger_time_;
        Int_t trigger_offset_;
        Int_t trigger_info_;
        Bool_t invalid_;
        // The Event tree
        TTree* event_tree_; // no unique_ptr, ROOT takes ownership

        // Output data file to write
        std::unique_ptr<TFile> output_file_;

        // retrieve message containing collected pixel hits for all detectors
        std::vector<std::shared_ptr<PixelHitMessage>> pixel_hit_messages_;
    };
} // namespace allpix
