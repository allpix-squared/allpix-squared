/**
 * @file
 * @brief Definition of generic charge propagation module
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <atomic>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include <Math/Point3D.h>
#include <TFile.h>
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

namespace allpix {
    using OutputPlotPoints = std::vector<std::pair<PropagatedCharge, std::vector<ROOT::Math::XYZPoint>>>;

    /**
     * @ingroup Modules
     * @brief Generic module for Runge-Kutta propagation of charge deposits in the sensitive device
     * @note This module supports parallelization
     *
     * Splits the sets of deposited charge in several sets which are propagated individually. The propagation consist of a
     * combination of drift from a charge mobility parameterization and diffusion using a Gaussian random walk process.
     * Propagation continues until the charge deposits 'leave' the sensitive device. Sets of charges do not interact with
     * each other and are threated fully separate, allowing for a speed-up by propagating the charges in multiple threads.
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
         * @brief Create output plots in every event
         * @param event_num Index for this event
         * @param output_plot_points List of points cached for plotting
         */
        void create_output_plots(uint64_t event_num, OutputPlotPoints& output_plot_points);

        /**
         * @brief Propagate a single set of charges through the sensor
         * @param pos Position of the deposit in the sensor
         * @param type Type of the carrier to propagate
         * @param initial_time Initial time passed before propagation starts in local time coordinates
         * @param random_generator Reference to the random number engine to be used
         * @param output_plot_points Reference to vector to hold points for line graph output plots
         * @return Pair of the point where the deposit ended after propagation and the time the propagation took
         */
        std::pair<ROOT::Math::XYZPoint, double> propagate(const ROOT::Math::XYZPoint& pos,
                                                          const CarrierType& type,
                                                          const double initial_time,
                                                          RandomNumberGenerator& random_generator,
                                                          OutputPlotPoints& output_plot_points) const;

        // Local copies of configuration parameters to avoid costly lookup:
        double temperature_{}, timestep_min_{}, timestep_max_{}, timestep_start_{}, integration_time_{},
            target_spatial_precision_{}, output_plots_step_{};
        bool output_plots_{}, output_linegraphs_{}, output_animations_{}, output_plots_lines_at_implants_{};
        bool propagate_electrons_{}, propagate_holes_{};
        unsigned int charge_per_step_{};

        // Models for electron and hole mobility and lifetime
        Mobility mobility_;
        Recombination recombination_;

        // Precalculated value for Boltzmann constant:
        double boltzmann_kT_;

        // Predefined values for electron/hole velocity calculation in magnetic fields
        double electron_Hall_;
        double hole_Hall_;

        // Magnetic field
        bool has_magnetic_field_;
        ROOT::Math::XYZVector magnetic_field_;

        // Statistical information
        std::atomic<unsigned int> total_propagated_charges_{};
        std::atomic<unsigned int> total_steps_{};
        std::atomic<long unsigned int> total_time_picoseconds_{};
        Histogram<TH1D> step_length_histo_;
        Histogram<TH1D> drift_time_histo_;
        Histogram<TH1D> uncertainty_histo_;
        Histogram<TH1D> group_size_histo_;
        Histogram<TH1D> recombine_histo_;
        std::mutex stats_mutex_;
    };

} // namespace allpix
