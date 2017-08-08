/**
 * @file
 * @brief Definition of simple charge transfer module
 * @copyright MIT License
 */

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

#include "objects/Pixel.hpp"
#include "objects/PixelCharge.hpp"
#include "objects/PropagatedCharge.hpp"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module that directly converts propagated charges to charges on a pixel
     *
     * This module does a simple direct mapping from propagated charges to the nearest pixel in the grid. It only considers
     * propagated charges within a certain distance from the implants and within the pixel grid, charges in the rest of the
     * sensor are ignored. The module combines all the propagated charges to a set of charges at a specific pixel.
     */
    class CapacitiveTransferModule : public Module {
    public:
        /**
         * @brief Constructor for this detector-specific module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        CapacitiveTransferModule(Configuration config, Messenger* messenger, std::shared_ptr<Detector> detector);

        /**
         * @brief Transfer the propagated charges to the pixels
         */
        void run(unsigned int) override;

        /**
         * @brief Display statistical summary
         */
        void finalize() override;

    private:
        Configuration config_;
        Messenger* messenger_;
        std::shared_ptr<Detector> detector_;
        std::shared_ptr<DetectorModel> model_;

        /**
         * @brief Compare two pixels, necessary to store them in the a std::map
         */
        struct pixel_cmp {
            bool operator()(const Pixel::Index& p1, const Pixel::Index& p2) const {
                if(p1.x() == p2.x()) {
                    return p1.y() < p2.y();
                }
                return p1.x() < p2.x();
            }
        };

        // Message containing the propagated charges
        std::shared_ptr<PropagatedChargeMessage> propagated_message_;

        // Statistical information
        unsigned int total_transferred_charges_{};
        std::set<Pixel::Index, pixel_cmp> unique_pixels_;
    };
} // namespace allpix
