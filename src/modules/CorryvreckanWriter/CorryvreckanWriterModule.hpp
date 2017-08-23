/**
 * @file
 * @brief Definition of CorryvreckanWriter module
 * @copyright MIT License
 *
 * Contains minimal dummy module to use as a start for the development of your own module
 *
 * Refer to the User's Manual for more details.
 */

#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

// Local includes
#include "Pixel.h"
#include "TestBeamObject.h"
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

    class CorryvreckanWriterModule : public Module {
    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         */
        CorryvreckanWriterModule(Configuration config, Messenger* messenger, GeometryManager* geo_manager);

        /**
         * @brief Set up output file and ntuple for filewriting
         */
        void init() override;

        /**
         * @brief Take the digitised pixel hits and write them into the output file
         */
        void run(unsigned int) override;

        /**
         * @brief Write output trees to file
         */
        void finalize() override;

    private:
        // General module members
        std::vector<std::shared_ptr<PixelHitMessage>> pixel_messages_; // Receieved pixels
        Messenger* messenger_;
        Configuration config_;
        GeometryManager* geometryManager_;

        // Parameters for output writing
        std::string fileName_;                                   // Output filename
        std::unique_ptr<TFile> outputFile_;                      // Output file
        long long int time_;                                     // Event time being written
        std::map<std::string, TTree*> outputTrees_;              // Output trees
        std::map<std::string, corryvreckan::Pixel*> treePixels_; // Objects attached to trees for writing
    };
} // namespace allpix
