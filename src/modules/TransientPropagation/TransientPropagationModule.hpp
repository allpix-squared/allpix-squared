/**
 * @file
 * @brief Definition of charge propagation module with transient behavior simulation
 * @copyright Copyright (c) 2019-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <string>

#include <Math/DisplacementVector2D.h>
#include <Math/Point3D.h>
#include <TH1D.h>
#include <TH2D.h>

#include "core/config/Configuration.hpp"
#include "core/geometry/DetectorModel.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

#include "objects/DepositedCharge.hpp"
#include "objects/Pulse.hpp"

#include "physics/Mobility.hpp"
#include "physics/Recombination.hpp"
#include "physics/Trapping.hpp"

#include "tools/ROOT.h"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module for simulation of transient current development using a Runge-Kutta approach
     *
     * This modules takes sets of deposited charge carriers and moves them through the electric field of the detector by
     * means of a Runge-Kutta integration. Diffusion is added after each step. The weighting potential different for each
     * step is calculated and the induced charge on neighboring pixels is deduced at every timestep in order to form a
     * transient current pulse.
     */
    class TransientPropagationModule : public Module {
    public:
        /**
         * @brief Constructor for this detector-specific module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        TransientPropagationModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);

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
        std::shared_ptr<DepositedChargeMessage> deposits_message_;

        /**
         * @brief Propagate a single set of charges through the sensor
         * @param event        Pointer to current event
         * @param pos          Position of the deposit in the sensor
         * @param type         Type of the carrier to propagate
         * @param charge       Total charge of the observed charge carrier set
         * @param initial_time Initial timestamp referring to the start of the event
         * @param pixel_map    Map of surrounding pixels and their induced pulses. Provided as reference to store simulation
         *                  result in
         * @return          Tuple of the point where the deposit ended after propagation, the time the propagation took and a
         * flag whether it is still alive or has recombined or was trapped
         */
        std::tuple<ROOT::Math::XYZPoint, double, bool, bool> propagate(Event* event,
                                                                       const ROOT::Math::XYZPoint& pos,
                                                                       const CarrierType& type,
                                                                       const unsigned int charge,
                                                                       const double initial_time,
                                                                       std::map<Pixel::Index, Pulse>& pixel_map);

        // Local copies of configuration parameters to avoid costly lookup:
        double temperature_{}, timestep_{}, integration_time_{};
        bool output_plots_{};
        ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> matrix_;
        unsigned int charge_per_step_{};
        unsigned int max_charge_groups_{};

        // Models for electron and hole mobility and lifetime
        Mobility mobility_;
        Recombination recombination_;
        Trapping trapping_;

        // Precalculated value for Boltzmann constant:
        double boltzmann_kT_;

        // Predefined values for electron/hole velocity calculation in magnetic fields
        double electron_Hall_;
        double hole_Hall_;

        // Magnetic field
        bool has_magnetic_field_{};
        ROOT::Math::XYZVector magnetic_field_;

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
    };
} // namespace allpix
