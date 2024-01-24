/**
 * @file
 * @brief Definition of a module to deposit charges at a specific point
 *
 * @copyright Copyright (c) 2018-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <string>

#include <TH2D.h>

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
            FIXED, ///< Deposition at a specific point
            SCAN,  ///< Scan through the volume of a pixel
            SPOT,  ///< Deposition around fixed position with Gaussian profile
        };

        /**
         * @brief Types of sources
         */
        enum class SourceType {
            POINT, ///< Deposition at a single point
            MIP,   ///< MIP-like linear deposition of charge carrier
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
         * @brief Initialize the histograms
         */
        void initialize() override;

        /**
         * @brief Deposit charge carriers for every simulated event
         */
        void run(Event*) override;

        /**
         * @brief Write output plots
         */
        void finalize() override;

    private:
        /**
         * @brief Helper function to deposit charges at a single point
         * @param event Pointer to current event
         * @param position
         */
        void DepositPoint(Event*, const ROOT::Math::XYZPoint& position);

        /**
         * @brief Helper function to deposit charges along a line
         * @param event Pointer to current event
         * @param position
         */
        void DepositLine(Event*, const ROOT::Math::XYZPoint& position);

        /**
         * @brief Finds and returns the points where a line with mip_direction through a given point intersects the sensor
         * @param line_origin Point the line goes through
         */
        std::tuple<ROOT::Math::XYZPoint, ROOT::Math::XYZPoint> SensorIntersection(const ROOT::Math::XYZPoint& line_origin);

        Messenger* messenger_;

        std::shared_ptr<Detector> detector_;
        std::shared_ptr<allpix::DetectorModel> detector_model_;

        DepositionModel model_;
        SourceType type_;
        double spot_size_{};
        ROOT::Math::XYZVector voxel_;
        double step_size_{};
        unsigned int root_{}, carriers_{};
        ROOT::Math::XYZVector position_{};
        ROOT::Math::XYZVector mip_direction_{};
        std::vector<std::string> scan_coordinates_{};
        size_t no_of_coordinates_;

        bool scan_x_;
        bool scan_y_;
        bool scan_z_;

        // Output plot parameters
        bool output_plots_{};
        int output_plots_bins_per_um_{};

        Histogram<TH2D> deposition_position_xy;
        Histogram<TH2D> deposition_position_xz;
        Histogram<TH2D> deposition_position_yz;
    };
} // namespace allpix
