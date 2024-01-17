/**
 * @file
 * @brief Definition of ProjectionPropagation module
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
#include "physics/Recombination.hpp"

#include "tools/ROOT.h"
#include "tools/line_graphs.h"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to project created electrons onto the sensor surface including diffusion
     *
     * The electrons from the deposition message are projected onto the sensor surface as a simple propagation method.
     * Diffusion is added by approximating the drift time and drawing a random number from a 2D gaussian distribution of the
     * calculated width.
     */
    class ProjectionPropagationModule : public Module {
    public:
        /**
         * @brief Constructor for this detector-specific module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        ProjectionPropagationModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);

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
        bool output_plots_{}, output_linegraphs_{};
        double integration_time_{};
        bool diffuse_deposit_;
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
        Recombination recombination_;

        // Precalculated value for Boltzmann constant:
        double boltzmann_kT_;

        // Statistical information
        std::atomic<unsigned int> total_deposits_{}, deposits_exceeding_max_groups_{};
        Histogram<TH1D> drift_time_histo_;
        Histogram<TH1D> diffusion_time_histo_;
        Histogram<TH1D> propagation_time_histo_;
        Histogram<TH1D> initial_position_histo_;
        Histogram<TH1D> recombine_histo_;
        Histogram<TH1D> group_size_histo_;
    };
} // namespace allpix
