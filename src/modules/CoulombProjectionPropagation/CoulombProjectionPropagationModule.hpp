/**
 * @file
 * @brief Definition of ProjectionPropagation module
 *
 * @copyright Copyright (c) 2017-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 *
 * Contains minimal dummy module to use as a start for the development of your own module
 *
 * Refer to the User's Manual for more details.
 */

#include <string>

#include <TF1.h>
#include <TH1D.h>

#include "core/config/Configuration.hpp"
#include "core/geometry/DetectorModel.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

#include "objects/DepositedCharge.hpp"
#include "objects/PropagatedCharge.hpp"

#include "physics/Mobility.hpp"
#include "physics/Recombination.hpp"

#include "tools/ROOT.h"
#include "tools/line_graphs.h"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to project created electrons onto the sensor surface including drift, diffusion, and repulsion; adapted
     * from ProjectionPropagationModule
     *
     *
     * The electrons/holes from the deposition message are projected onto the sensor surface following the GGD model
     * Approximated drift time t --> GGD parameters alpha a(t) and beta b(t) --> sample the position from the GGD
     * distribution with the given parameters
     * a(t), b(t) is hard coded, while the eight parameters are given in the configuration file as a string of eight
     * doubles, separated by commas. The order of the parameters is:
     * - a0, a1, a2, a3, b0, b1, b2, b3
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
         * @brief Projection of the electrons to the surface
         */
        void run(Event*) override;

        /**
         * @brief Write plots if needed
         */
        void finalize() override;

    private:
        Messenger* messenger_;
        std::shared_ptr<const Detector> detector_;
        std::shared_ptr<DetectorModel> model_;

        // Config parameters
        bool output_plots_{};
        double integration_time_{};
        unsigned int charge_per_step_{};
        unsigned int max_charge_groups_{};

        // Carrier type to be propagated
        CarrierType propagate_type_;
        // Side to propagate too
        double top_z_;

        // Precalculated values for electron and hole critical fields
        double hole_Ec_;
        double electron_Ec_;

        // Models for electron and hole mobility and lifetime
        std::unique_ptr<JacoboniCanali> mobility_;
        // Recombination recombination_;

        // Precalculated value for Boltzmann constant:
        double boltzmann_kT_;

        // Parameterized functions for alpha and beta of the Generalized Gaussian Distribution
        TF1* alpha_function_{nullptr};
        TF1* beta_function_{nullptr};
        std::array<double, 8> parameters_;

        // Statistical information
        std::atomic<unsigned int> total_deposits_{}, deposits_exceeding_max_groups_{};
        Histogram<TH1D> propagation_time_histo_;
        Histogram<TH1D> initial_position_histo_;
        Histogram<TH1D> group_size_histo_;
    };
} // namespace allpix
