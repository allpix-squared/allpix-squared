/**
 * Message for a deposited charge in a sensor
 *
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_DEPOSITED_CHARGE_H
#define ALLPIX_DEPOSITED_CHARGE_H

#include "SensorCharge.hpp"

namespace allpix {
    // object definition
    class DepositedCharge : public SensorCharge {
    public:
        DepositedCharge(ROOT::Math::XYZPoint position, unsigned int charge) : SensorCharge(std::move(position), charge) {}
    };

    // link to the carrying message
    using DepositedChargeMessage = Message<DepositedCharge>;
}

#endif
