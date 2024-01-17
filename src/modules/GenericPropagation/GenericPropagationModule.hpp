/**
 * @file
 * @brief Definition of generic charge propagation module
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
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
#include <TFile.h>
#include <TH1D.h>
#include <TProfile.h>

#include "core/config/Configuration.hpp"
#include "core/geometry/DetectorModel.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

#include "objects/DepositedCharge.hpp"
#include "objects/PropagatedCharge.hpp"

#include "physics/Detrapping.hpp"
#include "physics/ImpactIonization.hpp"
#include "physics/Mobility.hpp"
#include "physics/Recombination.hpp"
#include "physics/Trapping.hpp"

#include "tools/ROOT.h"
#include "tools/line_graphs.h"

namespace allpix {

    /**
     * @ingroup Modules
     * @brief Generic module for Runge-Kutta propagation of charge deposits in the sensitive device
     * @note This module supports multithreading
     *
     * Splits the sets of deposited charge in several sets which are propagated individually. The propagation consist of a
     * combination of drift from a charge mobility parameterization and diffusion using a Gaussian random walk process.
     * Propagation continues until the charge deposits 'leave' the sensitive device. Sets of charges do not interact with
     * each other and are treated fully separate, allowing for a speed-up by propagating the charges in multiple threads.
     */
    class GenericPropagationModule : public Module {
    public:
        /**
         * @brief Constructor for this detector-specific module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        GenericPropagationModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);

        /**
         * @brief Initialize the module and check field configuration
         */
        void initialize() override;

        /**
         * @brief Propagate all deposited charges through the sensor
         */
        void run(Event*) override;

        /**
         * @brief Write statistical summary
         */
        void finalize() override;

    private:
        Messenger* messenger_;
        std::shared_ptr<const Detector> detector_;
        std::shared_ptr<DetectorModel> model_;

        /**
         * @brief Propagate a single set of charges through the sensor
         * @param event               Pointer to current event
         * @param deposit             Reference to the original deposited charge object
         * @param pos                 Position of the deposit in the sensor
         * @param type                Type of the carrier to propagate
         * @param charge              Total charge of the observed charge carrier set
         * @param initial_time_local  Initial local time with respect to the start of the event
         * @param initial_time_global Initial global time with respect to the start of the event
         * @param level               Current level depth of the generated shower
         * @param propagated_charges  Reference to vector with all produced final PropagatedCharge objects
         * @param output_plot_points Reference to vector to hold points for line graph output plots
         *
         * @return Total recombined, trapped and propagated charge for statistics purposes
         */
        std::tuple<unsigned int, unsigned int, unsigned int, unsigned int, long double>
        propagate(Event* event,
                  const DepositedCharge& deposit,
                  const ROOT::Math::XYZPoint& pos,
                  const CarrierType& type,
                  unsigned int charge,
                  const double initial_time_local,
                  const double initial_time_global,
                  const unsigned int level,
                  std::vector<PropagatedCharge>& propagated_charges,
                  LineGraph::OutputPlotPoints& output_plot_points) const;

        // Local copies of configuration parameters to avoid costly lookup:
        double temperature_{}, timestep_min_{}, timestep_max_{}, timestep_start_{}, integration_time_{},
            target_spatial_precision_{}, output_plots_step_{};
        bool output_plots_{}, output_linegraphs_{}, output_linegraphs_collected_{}, output_linegraphs_recombined_{},
            output_linegraphs_trapped_{}, output_animations_{};
        bool propagate_electrons_{}, propagate_holes_{};
        unsigned int charge_per_step_{};
        unsigned int max_charge_groups_{};
        unsigned int max_multiplication_level_{};

        // Models for electron and hole mobility and lifetime
        Mobility mobility_;
        Recombination recombination_;
        ImpactIonization multiplication_;
        Trapping trapping_;
        Detrapping detrapping_;

        // Precalculated value for Boltzmann constant:
        double boltzmann_kT_;

        // Predefined values for electron/hole velocity calculation in magnetic fields
        double electron_Hall_;
        double hole_Hall_;

        // Magnetic field
        bool has_magnetic_field_;

        // Statistical information
        std::atomic<unsigned int> total_propagated_charges_{};
        std::atomic<unsigned int> total_steps_{};
        std::atomic<long unsigned int> total_time_picoseconds_{};
        std::atomic<unsigned int> total_deposits_{}, deposits_exceeding_max_groups_{};
        Histogram<TH1D> step_length_histo_;
        Histogram<TH1D> drift_time_histo_;
        Histogram<TH1D> uncertainty_histo_;
        Histogram<TH1D> group_size_histo_;
        Histogram<TH1D> recombine_histo_;
        Histogram<TH1D> trapped_histo_;
        Histogram<TH1D> recombination_time_histo_;
        Histogram<TH1D> trapping_time_histo_;
        Histogram<TH1D> detrapping_time_histo_;
        Histogram<TH1D> gain_primary_histo_;
        Histogram<TH1D> gain_all_histo_;
        Histogram<TH1D> gain_e_histo_;
        Histogram<TH1D> gain_h_histo_;
        Histogram<TH1D> multiplication_level_histo_;
        Histogram<TH1D> multiplication_depth_histo_;
        Histogram<TProfile> gain_e_vs_x_, gain_e_vs_y_, gain_e_vs_z_;
        Histogram<TProfile> gain_h_vs_x_, gain_h_vs_y_, gain_h_vs_z_;
    };

} // namespace allpix
