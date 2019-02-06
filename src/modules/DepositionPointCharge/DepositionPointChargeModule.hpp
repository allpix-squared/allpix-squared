/**
 * @file
 * @brief Definition of a module to deposit charges at a specific point
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <string>

#include "core/module/Module.hpp"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to deposit charges at predefined positions in the sensor volume
     *
     * This module can deposit charge carriers at defined positions inside the sensitive volume of the detector
     */
    class DepositionPointChargeModule : public Module {
        /**
         * @brief Types of deposition
         */
        enum class DepositionModel {
            NONE = 0, ///< No deposition
            POINT,    ///< Deposition at a specific point
            SCAN,     ///< Scan through the volume of a pixel
        };

    public:
        /**
         * @brief Constructor for a module to deposit charges at a specific point in the detector's active sensor volume
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        DepositionPointChargeModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);

        /**
         * @brief Deposit charge carriers for every simulated event
         */
        void run(unsigned int) override;

        /**
         * @brief Initialize the histograms
         */
        void init() override;

    private:
        std::shared_ptr<Detector> detector_;
        Messenger* messenger_;

        DepositionModel model_;
        ROOT::Math::XYZVector voxel_;
        unsigned int root_;
    };
} // namespace allpix
