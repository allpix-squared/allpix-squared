/**
 * Message for a deposited charge
 *
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_CHARGE_DEPOSIT_H
#define ALLPIX_CHARGE_DEPOSIT_H

#include <Math/Vector3D.h>

#include "core/messenger/Message.hpp"

namespace allpix {
    // type of the deposits
    class ChargeDeposit {
    public:
        ChargeDeposit(ROOT::Math::XYZVector position, unsigned int charge);

        ROOT::Math::XYZVector getPosition() const;
        unsigned int getCharge() const;

    private:
        ROOT::Math::XYZVector position_;
        unsigned int charge_;
    };

    // name for the carrying message
    using ChargeDepositMessage = Message<ChargeDeposit>;
} // namespace allpix

#endif /* ALLPIX_CHARGE_DEPOSIT_H */
