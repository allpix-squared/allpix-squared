/**
 * @file
 * @brief Definition of SphericalRepulsionPropagation module
 *
 * @copyright Copyright (c) 2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include <Math/Point3D.h>
#include <TH1D.h>
#include <TProfile.h>

#include "core/config/Configuration.hpp"
#include "core/geometry/DetectorModel.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

#include "objects/DepositedCharge.hpp"
#include "objects/PropagatedCharge.hpp"

#include "physics/Mobility.hpp"

#include "tools/ROOT.h"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Propagates deposited charge clouds including drift, diffusion and mutual Coulomb repulsion using a spherical
     * shell model
     *
     * All deposits of the same carrier type in an event form one charge cloud, initially a 3D Gaussian with a width from
     * the empirical photoelectron range. The cloud center drifts along the electric field, while the carriers move relative
     * to the center by diffusion and by the repulsion field of the charge enclosed in the sphere through their position
     * (Gauss' law). Carriers are grouped into sets of `charge_per_step` which are propagated together.
     * See https://doi.org/10.1016/j.nima.2025.170894.
     */
    class SphericalRepulsionPropagationModule : public Module {
    public:
        /**
         * @brief Constructor for this detector-specific module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        SphericalRepulsionPropagationModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);

        /**
         * @brief Initialize the physics models and create plots if needed
         */
        void initialize() override;

        /**
         * @brief Propagate the charge clouds of the event to the sensor surface
         */
        void run(Event*) override;

        /**
         * @brief Print statistics of the propagation
         */
        void finalize() override;

    private:
        /**
         * @brief Propagate one charge cloud and append the resulting charge groups to the output
         * @param event Pointer to the current event
         * @param cloud Deposits of one carrier type forming this cloud
         * @param type Carrier type of the cloud
         * @param propagated_charges Output vector of propagated charges
         * @return Number of charge carriers which have not been propagated into the output
         */
        unsigned int propagate_cloud(Event* event,
                                     const std::vector<const DepositedCharge*>& cloud,
                                     CarrierType type,
                                     std::vector<PropagatedCharge>& propagated_charges);

        Messenger* messenger_;
        std::shared_ptr<const Detector> detector_;
        std::shared_ptr<DetectorModel> model_;

        // Config parameters
        bool output_plots_{};
        bool propagate_electrons_{};
        bool propagate_holes_{};
        double timestep_{};
        double integration_time_{};
        double temperature_{};
        double charge_creation_energy_{};
        unsigned int charge_per_step_{};
        unsigned int max_charge_groups_{};

        // Mobility model
        Mobility mobility_;

        // Precalculated values for Boltzmann constant and Coulomb constant 1 / (4 pi eps0 eps_r):
        double boltzmann_kT_{};
        double coulomb_constant_{};

        // Collection surfaces of the sensor in local coordinates
        double sensor_min_z_{};
        double sensor_max_z_{};

        // Statistical information
        std::atomic<unsigned int> clouds_exceeding_max_groups_{};
        std::atomic<unsigned int> total_clouds_{}, total_propagated_charges_{}, total_lost_charges_{};
        Histogram<TH1D> drift_time_histo_;
        Histogram<TH1D> group_size_histo_;
        Histogram<TH1D> cloud_charge_histo_;
        Histogram<TProfile> cloud_rms_vs_depth_;
    };
} // namespace allpix
