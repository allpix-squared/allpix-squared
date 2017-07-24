/**
 * @file
 * @brief Definition of propagated charge object
 * @copyright MIT License
 */

#ifndef ALLPIX_PROPAGATED_CHARGE_H
#define ALLPIX_PROPAGATED_CHARGE_H

#include "DepositedCharge.hpp"
#include "SensorCharge.hpp"

namespace allpix {
    /**
     * @ingroup Objects
     * @brief Set of charges propagated through the sensor
     */
    class PropagatedCharge : public SensorCharge {
    public:
        /**
         * @brief Construct a set of propagated charges
         * @param local_position Local position of the propagated set of charges in the sensor
         * @param global_position Global position of the propagated set of charges in the sensor
         * @param charge Total charge propagated
         * @param event_time Total time of propagation arrival after event start
         * @param deposited_charge Optional pointer to related deposited charge
         */
        PropagatedCharge(ROOT::Math::XYZPoint local_position,
                         ROOT::Math::XYZPoint global_position,
                         CarrierType type,
                         unsigned int charge,
                         double event_time,
                         const DepositedCharge* deposited_charge = nullptr);

        /**
         * @brief Get related deposited charge
         * @return Pointer to possible deposited charge
         */
        const DepositedCharge* getDepositedCharge() const;

        /**
         * @brief ROOT class definition
         */
        ClassDef(PropagatedCharge, 2);
        /**
         * @brief Default constructor for ROOT I/O
         */
        PropagatedCharge() = default;

    private:
        TRef deposited_charge_;
    };

    /**
     * @brief Typedef for message carrying propagated charges
     */
    using PropagatedChargeMessage = Message<PropagatedCharge>;
}

#endif
