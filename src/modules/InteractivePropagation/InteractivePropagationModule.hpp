/**
 * @file
 * @brief Definition of [InteractivePropagation] module
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 *
 * Contains minimal dummy module to use as a start for the development of your own module
 *
 * Please refer to the User Manual for more details on the different files of a module and the methods of the module class..
 */

#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

#include "objects/DepositedCharge.hpp"

#include "../TransientPropagation/TransientPropagationModule.hpp"
#include <TGraph.h>
#include <TMultiGraph.h>

namespace allpix {
    /**
     * @ingroup Modules
     * @brief A Module that implements Coulomb repulsion between charges. (Based on TransientPropagationModule)
     *
     */
    // class InteractivePropagationModule : public TransientPropagationModule {
    class InteractivePropagationModule : public Module {
    public:

        /**
         * @brief Constructor for this detector-specific module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        InteractivePropagationModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);

        /**
         * @brief Initialize the module and check field configuration
         */
        void initialize() override;

        /**
         * @brief Propagate all deposited charges through the sensor
         */
        void run(Event*) override;

        /**
         * @brief Write statistical summary and histograms
         */
        void finalize() override;  
    
    private:
        Messenger* messenger_;

        // General module members
        std::shared_ptr<const Detector> detector_;
        std::shared_ptr<DetectorModel> model_;

        /**
         * @brief Propagate a vector containing all charges through the sensor synchronously
         * @param event                 Pointer to current event
         * @param propagating_charges   Reference to vector containing info about all of the in-progress charges
         * @param propagated_charges    Reference to vector with all produced final PropagatedCharge objects
         * @param output_plot_points    Reference to vector to hold points for line graph output plots
         * 
         * @return Total recombined, trapped and propagated charge for statistics purposes
         */
        std::tuple<unsigned int, unsigned int, unsigned int> 
        propagate_together(Event* event,
                           std::vector<PropagatedCharge>& propagating_charges,
                           std::vector<PropagatedCharge>& propagated_charges,
                           LineGraph::OutputPlotPoints& output_plot_points) const;

        // Local copies of configuration parameters to avoid costly lookup:
        double temperature_{}, timestep_{}, integration_time_{}, output_plots_step_{};
        bool output_plots_{}, output_linegraphs_{}, output_linegraphs_collected_{}, output_linegraphs_recombined_{},
            output_linegraphs_trapped_{}, output_rms_{};
        unsigned int distance_{};
        unsigned int charge_per_step_{};
        unsigned int max_charge_groups_{}; // Changed to be the max out of all deposits (may be exceeded due to deposits with very few charges)

        unsigned int max_multiplication_level_{};

        double relative_permittivity_{};

        // z bounds for capacitor (set during initialization)
        double z_lim_pos_;
        double z_lim_neg_;

        // Maximum magnitude of the field between two charges
        double coulomb_field_limit_{};
        double coulomb_distance_limit_squared_{};
        // double coulomb_field_limit_squared_{};

        // Configurability of diffusion and coulomb repulsion
        bool enable_diffusion_{};
        bool enable_coulomb_repulsion_{};

        // Determines whether electrons, holes, or both are included in the propagation. Default to true.
        bool propagate_electrons_{};
        bool propagate_holes_{};

        // Toggle for whether to ignore mirror charges
        bool include_mirror_charges_{};

        // Models for electron and hole mobility and lifetime
        Mobility mobility_;
        Recombination recombination_;
        ImpactIonization multiplication_;
        Trapping trapping_;
        Detrapping detrapping_;

        // Precalculated value for Boltzmann constant:
        double boltzmann_kT_;

        // Predefined value for Coulomb constant in units MV mm e-1
        double coulomb_K_;

        // Predefined values for electron/hole velocity calculation in magnetic fields
        double electron_Hall_;
        double hole_Hall_;

        // Reflectivity of sensor surface (outside implants)
        double surface_reflectivity_{0};

        // Magnetic field
        bool has_magnetic_field_{};

        // Deposit statistics
        std::atomic<unsigned int> total_deposits_{}, deposits_exceeding_max_groups_{};

        // Output plots
        Histogram<TH1D> potential_difference_, induced_charge_histo_, induced_charge_e_histo_, induced_charge_h_histo_;
        Histogram<TH2D> induced_charge_vs_depth_histo_, induced_charge_e_vs_depth_histo_, induced_charge_h_vs_depth_histo_;
        Histogram<TH2D> induced_charge_map_, induced_charge_e_map_, induced_charge_h_map_;
        Histogram<TH1D> step_length_histo_, group_size_histo_;
        Histogram<TH1D> drift_time_histo_;
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
        Histogram<TH1D> induced_charge_primary_histo_, induced_charge_primary_e_histo_, induced_charge_primary_h_histo_;
        Histogram<TH1D> induced_charge_secondary_histo_, induced_charge_secondary_e_histo_,
            induced_charge_secondary_h_histo_;
        TGraph *rms_e_subgraph_, *rms_h_subgraph_, *rms_x_e_subgraph_, *rms_y_e_subgraph_, *rms_z_e_subgraph_;
        TMultiGraph *rms_e_graph_, *rms_total_graph_;
        Histogram<TH1D> coulomb_mag_histo_;
    };

} // namespace allpix
