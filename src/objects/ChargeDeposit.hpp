/**
 * Message for a deposited charge
 *
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_CHARGE_DEPOSIT_H
#define ALLPIX_CHARGE_DEPOSIT_H

#include <Math/Point3D.h>

#include "core/messenger/Message.hpp"

namespace allpix {
    // type of the deposits
    class ChargeDeposit {
    public:
        ChargeDeposit(ROOT::Math::XYZVector position, unsigned int charge);

        // FIXME: should position be in local coordinates or global coordinates?
        ROOT::Math::XYZPoint getPosition() const;
        unsigned int getCharge() const;

    private:
        ROOT::Math::XYZPoint position_;
        unsigned int charge_;
    };

    // name for the carrying message
    using ChargeDepositMessage = Message<ChargeDeposit>;
} // namespace allpix

#endif /* ALLPIX_CHARGE_DEPOSIT_H */
