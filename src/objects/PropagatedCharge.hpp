/**
 * Message for a propagated charge in a sensor
 *
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_PROPAGATED_CHARGE_H
#define ALLPIX_PROPAGATED_CHARGE_H

#include "SensorCharge.hpp"

namespace allpix {
    // object definition
    class PropagatedCharge : public SensorCharge {
    public:
        PropagatedCharge(ROOT::Math::XYZPoint position, unsigned int charge, long double event_time)
            : SensorCharge(std::move(position), charge, event_time) {}

        ClassDef(PropagatedCharge, 1);
        PropagatedCharge() = default;
    };

    // link to the carrying message
    using PropagatedChargeMessage = Message<PropagatedCharge>;
}

#endif
