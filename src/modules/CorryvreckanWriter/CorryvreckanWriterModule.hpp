/**
 * @file
 * @brief Definition of CorryvreckanWriter module
 * @copyright Copyright (c) 2017-2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

// Local includes
#include "corryvreckan/MCParticle.h"
#include "corryvreckan/Object.hpp"
#include "corryvreckan/Pixel.h"
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

    class CorryvreckanWriterModule : public WriterModule {
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
        void init(std::mt19937_64&) override;

        /**
         * @brief Take the digitised pixel hits and write them into the output file
         */
        void run(Event*) override;

        /**
         * @brief Write output trees to file
         */
        void finalize() override;

    private:
        // General module members
        Messenger* messenger_;
        GeometryManager* geometryManager_;

        // Parameters for output writing
        std::string fileName_;                                   // Output filename
        std::string geometryFileName_;                           // Output geometry filename
        std::unique_ptr<TFile> outputFile_;                      // Output file
        long long int time_;                                     // Event time being written
        std::map<std::string, TTree*> outputTrees_;              // Output trees
        std::map<std::string, corryvreckan::Pixel*> treePixels_; // Objects attached to trees for writing

        bool outputMCtruth_;                                               // Decision to write out MC particle info
        std::map<std::string, TTree*> outputTreesMC_;                      // Output trees for MC particles
        std::map<std::string, corryvreckan::MCParticle*> treeMCParticles_; // Objects attached to trees for writing
    };
} // namespace allpix
