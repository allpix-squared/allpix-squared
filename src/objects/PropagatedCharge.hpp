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
        PropagatedCharge(ROOT::Math::XYZVector position, unsigned int charge) : SensorCharge(position, charge) {}
    };

    // link to the carrying message
    using PropagatedChargeMessage = Message<PropagatedCharge>;
}

#endif
