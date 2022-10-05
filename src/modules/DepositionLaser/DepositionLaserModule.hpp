/**
 * @file
 * @brief Definition of [DepositionLaser] module
 *
 * @copyright Copyright (c) 2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 *
 * Contains minimal dummy module to use as a start for the development of your own module
 *
 * Refer to the User's Manual for more details.
 */

#include <optional>
#include <string>
#include <utility>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to do function
     *
     * More detailed explanation of module
     */
    class DepositionLaserModule : public Module {
    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         */
        DepositionLaserModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager);

        /**
         * @brief [Initialise this module]
         */
        void initialize() override;

        /**
         * @brief [Run the function of this module]
         */
        void run(Event* event) override;

    private:
        std::optional<std::pair<double, double>> get_intersection(const std::shared_ptr<const Detector>& detector,
                                                                  const ROOT::Math::XYZPoint& position_global,
                                                                  const ROOT::Math::XYZVector& direction_global) const;

        // General module members
        GeometryManager* geo_manager_;
        Messenger* messenger_;

        // Source parameters
        ROOT::Math::XYZPoint source_position_{};
        ROOT::Math::XYZVector beam_direction_{};
        double beam_waist_;
        std::optional<double> beam_convergence_ = std::nullopt;
        std::optional<double> focal_distance_ = std::nullopt;
        size_t photon_number_;
        bool verbose_tracking_;
        double wavelength_;
        double absorption_length_;
    };
} // namespace allpix
