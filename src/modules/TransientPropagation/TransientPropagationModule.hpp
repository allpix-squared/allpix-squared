/**
 * @file
 * @brief Definition of charge propagation module with transient behavior simulation
 * @copyright Copyright (c) 2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <string>

#include <Math/DisplacementVector2D.h>
#include <Math/Point3D.h>
#include <TH1D.h>

#include "core/config/Configuration.hpp"
#include "core/geometry/DetectorModel.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

#include "objects/DepositedCharge.hpp"
#include "objects/Pulse.hpp"
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
        void init(std::mt19937_64&) override;

        /**
         * @brief Propagate all deposited charges through the sensor
         */
        void run(Event*) override;

        /**
         * @brief Write statistical summary and histograms
         */
        void finalize() override;

    private:
        // General module members
        std::shared_ptr<const Detector> detector_;
        std::shared_ptr<DetectorModel> model_;
        std::shared_ptr<DepositedChargeMessage> deposits_message_;

        /**
         * @brief Propagate a single set of charges through the sensor
         * @param event     Pointer to current event
         * @param pos       Position of the deposit in the sensor
         * @param type      Type of the carrier to propagate
         * @param charge    Total charge of the observed charge carrier set
         * @param pixel_map Map of surrounding pixels and their induced pulses. Provided as reference to store simulation
         *                  result in
         * @return          Pair of the point where the deposit ended after propagation and the time the propagation took
         */
        std::pair<ROOT::Math::XYZPoint, double> propagate(Event* event,
                                                          const ROOT::Math::XYZPoint& pos,
                                                          const CarrierType& type,
                                                          const unsigned int charge,
                                                          std::map<Pixel::Index, Pulse>& pixel_map);

        // Local copies of configuration parameters to avoid costly lookup:
        double temperature_{}, timestep_{}, integration_time_{};
        bool output_plots_{};
        ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> matrix_;

        // Precalculated values for electron and hole mobility
        double electron_Vm_;
        double electron_Ec_;
        double electron_Beta_;
        double hole_Vm_;
        double hole_Ec_;
        double hole_Beta_;

        // Precalculated value for Boltzmann constant:
        double boltzmann_kT_;

        // Predefined values for electron/hole velocity calculation in magnetic fields
        double electron_Hall_;
        double hole_Hall_;

        // Magnetic field
        bool has_magnetic_field_;
        ROOT::Math::XYZVector magnetic_field_;

        // Output plots
        TH1D *potential_difference_, *induced_charge_histo_, *induced_charge_e_histo_, *induced_charge_h_histo_;
        TH1D* step_length_histo_;
        TH1D* drift_time_histo_;
    };
} // namespace allpix
