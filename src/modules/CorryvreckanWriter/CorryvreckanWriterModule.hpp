/**
 * @file
 * @brief Definition of CorryvreckanWriter module
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

// Local includes
#include "corryvreckan/Event.hpp"
#include "corryvreckan/MCParticle.hpp"
#include "corryvreckan/Object.hpp"
#include "corryvreckan/Pixel.hpp"
#include "objects/PixelHit.hpp"

// ROOT includes
#include "TFile.h"
#include "TTree.h"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to do function
     *
     * More detailed explanation of module
     */

    class CorryvreckanWriterModule : public SequentialModule {
    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         */
        CorryvreckanWriterModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager);

        /**
         * @brief Set up output file and ntuple for filewriting
         */
        void initialize() override;

        /**
         * @brief Take the digitised pixel hits and write them into the output file
         */
        void run(Event* event) override;

        /**
         * @brief Write output trees to file
         */
        void finalize() override;

    private:
        // General module members
        Messenger* messenger_;
        GeometryManager* geometryManager_;

        // Parameters for output writing
        std::string fileName_;               // Output filename
        std::string geometryFileName_;       // Output geometry filename
        std::unique_ptr<TFile> output_file_; // Output file
        double time_{};                      // Event time being written
        bool output_mc_truth_{};             // Decision to write out MC
        bool timing_global_{};               // Decision to write global or local timestamps

        std::string reference_;
        std::vector<std::string> dut_;

        std::unique_ptr<TTree> event_tree_;
        corryvreckan::Event* event_{};

        std::unique_ptr<TTree> pixel_tree_;
        std::unique_ptr<TTree> mcparticle_tree_;
        std::map<std::string, std::vector<corryvreckan::Pixel*>*> write_list_px_;
        std::map<std::string, std::vector<corryvreckan::MCParticle*>*> write_list_mcp_;
    };
} // namespace allpix
