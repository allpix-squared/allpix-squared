/**
 * @file
 * @brief Definition of CoulombProjectionPropagation module
 *
 * @copyright Copyright (c) 2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <array>
#include <atomic>
#include <memory>
#include <string>

#include <TH1D.h>

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
     * @brief Module to project charge carriers onto the sensor surface including drift, diffusion and Coulomb repulsion;
     * adapted from ProjectionPropagationModule
     *
     * The lateral distribution of the collected charge carriers is modeled by a Generalized Gaussian Distribution (GGD)
     * with scale parameter alpha and shape parameter beta. Both are parameterized as functions of the approximated drift
     * time t, see https://doi.org/10.1016/j.nima.2025.170894:
     * - alpha(t) = a0 + a1 * sqrt(t) + a2 * t + a3 * t^2
     * - beta(t) = 2 + b0 * (t - b1)^b2 + b3 * exp(b4 * t)
     * The nine parameters a0-a3 and b0-b4 are provided via the configuration, with alpha in um and t in ns.
     */
    class CoulombProjectionPropagationModule : public Module {
    public:
        /**
         * @brief Constructor for this detector-specific module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        CoulombProjectionPropagationModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);

        /**
         * @brief Initialize - create plots if needed
         */
        void initialize() override;

        /**
         * @brief Projection of the charge carriers to the surface
         */
        void run(Event*) override;

        /**
         * @brief Print statistics of the propagation
         */
        void finalize() override;

    private:
        /**
         * @brief Scale parameter alpha of the GGD as function of the drift time
         * @param drift_time Approximated drift time in framework units
         * @return Scale parameter in framework units
         */
        double ggd_alpha(double drift_time) const;

        /**
         * @brief Shape parameter beta of the GGD as function of the drift time
         * @param drift_time Approximated drift time in framework units
         * @return Shape parameter
         */
        double ggd_beta(double drift_time) const;

        Messenger* messenger_;
        std::shared_ptr<const Detector> detector_;
        std::shared_ptr<DetectorModel> model_;

        // Config parameters
        bool output_plots_{};
        double integration_time_{};
        unsigned int charge_per_step_{};
        unsigned int max_charge_groups_{};

        // Carrier type to be propagated
        CarrierType propagate_type_{CarrierType::ELECTRON};
        // Side to propagate too
        double top_z_{};

        // Precalculated values for electron and hole critical fields
        double hole_Ec_{};
        double electron_Ec_{};

        // Mobility model, fixed to Jacoboni-Canali as used for the parameterization
        std::unique_ptr<JacoboniCanali> mobility_;

        // Parameters of alpha(t) and beta(t) of the Generalized Gaussian Distribution
        std::array<double, 4> alpha_parameters_{};
        std::array<double, 5> beta_parameters_{};

        // Statistical information
        std::atomic<unsigned int> total_deposits_{}, deposits_exceeding_max_groups_{};
        Histogram<TH1D> propagation_time_histo_;
        Histogram<TH1D> initial_position_histo_;
        Histogram<TH1D> group_size_histo_;
    };
} // namespace allpix
