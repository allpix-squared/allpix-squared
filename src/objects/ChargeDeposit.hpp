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
        ChargeDeposit(ROOT::Math::XYZVector position, double energy);

        ROOT::Math::XYZVector getPosition() const;
        double getEnergy() const;

    private:
        ROOT::Math::XYZVector position_;
        double energy_;
    };

    // name for the carrying message
    using ChargeDepositMessage = Message<ChargeDeposit>;
}

#endif /* ALLPIX_CHARGE_DEPOSIT_H */
