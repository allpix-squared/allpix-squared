/**
 * @file
 * @brief Definition of a module to deposit charges at a specific point
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
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
            FIXED,    ///< Deposition at a specific point
            SCAN,     ///< Scan through the volume of a pixel
            SPOT,     ///< Deposition around fixed position with Gaussian profile
        };

        /**
         * @brief Types of sources
         */
        enum class SourceType {
            NONE = 0, ///< No source
            POINT,    ///< Deposition at a single point
            MIP,      ///< MIP-like linear deposition of charge carrier
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
        /**
         * @brief Helper function to deposit charges at a single point
         * @param event event number
         */
        void DepositPoint(const ROOT::Math::XYZPoint& position);

        /**
         * @brief Helper function to deposit charges along a line
         * @param event event number
         */
        void DepositLine(const ROOT::Math::XYZPoint& position);

        std::shared_ptr<Detector> detector_;
        Messenger* messenger_;

        std::mt19937_64 random_generator_;

        DepositionModel model_;
        SourceType type_;
        double spot_size_{};
        ROOT::Math::XYZVector voxel_;
        double step_size_z_{};
        unsigned int root_{}, carriers_{};
    };
} // namespace allpix
