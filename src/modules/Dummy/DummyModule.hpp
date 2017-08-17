/**
 * @file
 * @brief Definition of [Dummy] module
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

#include "objects/PixelHit.hpp"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to do function
     *
     * More detailed explanation of module
     */
    class DummyModule : public Module {
    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         */
        DummyModule(Configuration config, Messenger* messenger, GeometryManager* geo_manager);

        /**
         * @brief [Initialise this module]
         */
        void init() override;

        /**
         * @brief [Run the function of this module]
         */
        void run(unsigned int) override;

    private:
        // General module members
        GeometryManager* geometryManager_;
        Configuration config_;
        Messenger* messenger_;
        std::vector<std::shared_ptr<PixelHitMessage>> pixel_messages_;
    };
} // namespace allpix
