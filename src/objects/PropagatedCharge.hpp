/**
 * Message for a propagated charge in a sensor
 *
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_PROPAGATED_CHARGE_H
#define ALLPIX_PROPAGATED_CHARGE_H

#include "DepositedCharge.hpp"
#include "SensorCharge.hpp"

namespace allpix {
    // object definition
    class PropagatedCharge : public SensorCharge {
    public:
        PropagatedCharge(ROOT::Math::XYZPoint position,
                         unsigned int charge,
                         double event_time,
                         const DepositedCharge* deposited_charge = nullptr);

        const DepositedCharge* getDepositedCharge() const;

        ClassDef(PropagatedCharge, 1);
        PropagatedCharge() = default;

    private:
        TRef deposited_charge_;
    };

    // link to the carrying message
    using PropagatedChargeMessage = Message<PropagatedCharge>;
}

#endif
