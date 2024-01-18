/**
 * @file
 * @brief Definition of [DepositionLaser] module
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 *
 * Contains minimal dummy module to use as a start for the development of your own module
 *
 * Refer to the User's Manual for more details.
 */

#include <map>
#include <optional>
#include <string>
#include <utility>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

#include "TH1D.h"
#include "TH2D.h"
#include "TH3D.h"
#include "tools/ROOT.h"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to do function
     *
     * More detailed explanation of module
     */
    class DepositionLaserModule : public Module {

        enum class BeamGeometry {
            CYLINDRICAL,
            CONVERGING,
        };

        // Data to return from tracking algorithms
        struct PhotonHit { // NOLINT
            std::shared_ptr<Detector> detector;
            ROOT::Math::XYZPoint entry_global;
            ROOT::Math::XYZPoint hit_global;
            double time_to_entry;
            double time_to_hit;
        };

    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         */
        DepositionLaserModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager);

        /**
         * @brief Initialization method of the module
         */
        void initialize() override;

        /**
         * @brief Run method of the module
         */
        void run(Event* event) override;

        /**
         * @brief Finalize and write optional histograms
         */
        void finalize() override;

    private:
        /**
         * @brief Check intersection of the given track with the given detector
         * This is a wrapper around LiangBarsky::intersectionDistances,
         * which properly transforms coordinates to make it work
         */
        std::optional<std::pair<double, double>> intersect_with_sensor(const std::shared_ptr<const Detector>& detector,
                                                                       const ROOT::Math::XYZPoint& position_global,
                                                                       const ROOT::Math::XYZVector& direction_global) const;

        /**
         * @brief Check intersection with passive objects if there is any
         * Only works for boxes
         */
        std::optional<std::pair<double, std::string>>
        intersect_with_passives(const ROOT::Math::XYZPoint& position_global,
                                const ROOT::Math::XYZVector& direction_global) const;

        /**
         * @brief Get a normal vector for a point where the given track enters the given detector
         * Returns a normal vector to sensor face, closest to the hit_point
         */
        ROOT::Math::XYZVector intersection_normal_vector(const std::shared_ptr<const Detector>& detector,
                                                         const ROOT::Math::XYZPoint& position_global) const;

        /**
         * @brief Generate starting position and direction for a single photon, obeying the set beam geometry
         * @param event is passed to get proper RNGs
         * Also fills histograms
         */
        std::pair<ROOT::Math::XYZPoint, ROOT::Math::XYZVector> generate_photon_geometry(Event* event);

        /**
         * @brief Track a photon, starting at the given point
         * version 2: refraction
         */
        std::optional<PhotonHit>
        track(const ROOT::Math::XYZPoint& position, const ROOT::Math::XYZVector& direction, double penetration_depth) const;

        // General module members
        GeometryManager* geo_manager_;
        Messenger* messenger_;

        // Laser parameters
        ROOT::Math::XYZPoint source_position_{};
        ROOT::Math::XYZVector beam_direction_{};
        double beam_waist_;

        BeamGeometry beam_geometry_{};
        double beam_convergence_angle_;
        double focal_distance_;

        size_t number_of_photons_;
        double wavelength_{0.};
        double absorption_length_{0.};
        double refractive_index_{0.};
        double pulse_duration_;
        bool is_user_optics_{false};

        size_t group_photons_;

        // Histograms
        bool output_plots_;
        Histogram<TH2D> h_intensity_sourceplane_{};
        Histogram<TH2D> h_intensity_focalplane_{};
        Histogram<TH1D> h_angular_phi_{};
        Histogram<TH1D> h_angular_theta_{};
        Histogram<TH1D> h_pulse_shape_{};
        std::map<const std::shared_ptr<Detector>, Histogram<TH3D>> h_deposited_charge_shapes_;
    };
} // namespace allpix
