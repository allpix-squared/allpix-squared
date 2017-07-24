/**
 * @file
 * @brief Definition of [LCIOWriter] module
 * @copyright MIT License
 *
 * Contains minimal dummy module to use as a start for the development of your own module
 *
 * Refer to the User's Manual for more details.
 */

#include <string>
#include <vector>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

#include "objects/PixelHit.hpp"

#include <IO/LCWriter.h>

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to do function
     *
     * More detailed explanation of module
     */
    class LCIOWriterModule : public Module {
    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         */
        LCIOWriterModule(Configuration config, Messenger* messenger, GeometryManager* geo_manager);

        /**
         * @brief [Run the function of this module]
         */
        void run(unsigned int) override;

        /**
         * @brief Close the output file
         */
        void finalize() override;

    private:
        Configuration config_;
        std::vector<std::shared_ptr<PixelHitMessage>> pixel_messages_;
        IO::LCWriter* lcWriter;
        std::map<std::string, unsigned int> detectorIDs;
    };
} // namespace allpix
