/**
 * @file
 * @brief Definition of [ProjectionPropagation] module
 * @copyright MIT License
 *
 * Contains minimal dummy module to use as a start for the development of your own module
 *
 * Refer to the User's Manual for more details.
 */

#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/DetectorModel.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

#include "objects/DepositedCharge.hpp"
#include "objects/PropagatedCharge.hpp"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to do function
     *
     * More detailed explanation of module
     */
    class ProjectionPropagationModule : public Module {
    public:
        /**
         * @brief Constructor for this detector-specific module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        ProjectionPropagationModule(Configuration config, Messenger* messenger, std::shared_ptr<Detector> detector);

        /**
         * @brief [Run the function of this module]
         */
        void run(unsigned int) override;

    private:
        Configuration config_;
        Messenger* messenger_;
        std::shared_ptr<const Detector> detector_;
        std::shared_ptr<DetectorModel> model_;

        // Precalculated values for electron and hole mobility
        double electron_Vm_;
        double electron_Ec_;
        double electron_Beta_;
        double hole_Vm_;
        double hole_Ec_;
        double hole_Beta_;

        // Local copies of configuration parameters to avoid costly lookup:
        double spatial_precision_{};

        // Precalculated value for Boltzmann constant:
        double boltzmann_kT_;

        // Deposits for the bound detector in this event
        std::shared_ptr<DepositedChargeMessage> deposits_message_;
    };
} // namespace allpix
