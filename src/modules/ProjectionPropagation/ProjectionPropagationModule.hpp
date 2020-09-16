/**
 * @file
 * @brief Definition of ProjectionPropagation module
 * @copyright MIT License
 *
 * Contains minimal dummy module to use as a start for the development of your own module
 *
 * Refer to the User's Manual for more details.
 */

#include <random>
#include <string>

#include <TH1D.h>

#include "core/config/Configuration.hpp"
#include "core/geometry/DetectorModel.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

#include "objects/DepositedCharge.hpp"
#include "objects/PropagatedCharge.hpp"

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
        void init() override;

        /**
         * @brief Projection of the electrons to the surface
         */
        void run(unsigned int) override;

        /**
         * @brief Write plots if needed
         */
        void finalize() override;

    private:
        Messenger* messenger_;
        std::shared_ptr<const Detector> detector_;
        std::shared_ptr<DetectorModel> model_;

        // Random generator for diffusion calculation
        std::mt19937_64 random_generator_;

        // Config parameters: Check whether plots should be generated
        bool output_plots_;
        double integration_time_{};
        bool diffuse_deposit_;

        // Carrier type to be propagated
        CarrierType propagate_type_;
        // Side to propagate too
        double top_z_;

        // Precalculated values for electron and hole mobility
        double hole_Vm_;
        double hole_Ec_;
        double hole_Beta_;
        double electron_Vm_;
        double electron_Ec_;
        double electron_Beta_;

        // Doping profile available?
        bool has_doping_profile_;

        // Calculated slope of the electric field
        double slope_efield_;

        // Precalculated value for Boltzmann constant:
        double boltzmann_kT_;

        // Predefined values for reference charge carrier lifetime and doping concentration
        double electron_lifetime_reference_;
        double hole_lifetime_reference_;
        double electron_doping_reference_;
        double hole_doping_reference_;

        // Output plot for drift time
        TH1D* drift_time_histo_;
        TH1D* diffusion_time_histo_;
        TH1D* propagation_time_histo_;
        TH1D* initial_position_histo_;

        // Deposits for the bound detector in this event
        std::shared_ptr<DepositedChargeMessage> deposits_message_;
    };
} // namespace allpix
