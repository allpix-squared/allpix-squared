/**
 * @file
 * @brief Definition of simple charge transfer module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <TH1D.h>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

#include "objects/Pixel.hpp"
#include "objects/PixelCharge.hpp"
#include "objects/PropagatedCharge.hpp"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module that directly converts propagated charges to charges on a pixel
     * @note This module supports parallelization
     *
     * This module does a simple direct mapping from propagated charges to the nearest pixel in the grid. It only considers
     * propagated charges within a certain distance from the implants and within the pixel grid, charges in the rest of the
     * sensor are ignored. The module combines all the propagated charges to a set of charges at a specific pixel.
     */
    class SimpleTransferModule : public Module {
    public:
        /**
         * @brief Constructor for this detector-specific module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        SimpleTransferModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);

        /**
         * @brief Initialize - check for field configuration and implants
         */
        void init(std::mt19937_64&) override;

        /**
         * @brief Transfer the propagated charges to the pixels
         */
        void run(Event*) const override;

        /**
         * @brief Display statistical summary
         */
        void finalize() override;

        /**
         * @brief Checks if the module is ready to run in the given event
         * @param Event pointer to the event the module will run
         */
        virtual bool isSatisfied(Event* event) const override;

    private:
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

        TH1D* drift_time_histo;

        // Flag whether to store output plots:
        bool output_plots_{};

        // Statistical information
        mutable unsigned int total_transferred_charges_{};
        mutable std::set<Pixel::Index, pixel_cmp> unique_pixels_;
    };
} // namespace allpix
