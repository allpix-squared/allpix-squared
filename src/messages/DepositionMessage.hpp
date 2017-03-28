/**
 * Message for a deposited charge
 *
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MESSAGE_DEPOSITION_H
#define ALLPIX_MESSAGE_DEPOSITION_H

#include <memory>
#include <string>
#include <vector>

#include <Math/Vector3D.h>

#include "core/messenger/Message.hpp"

namespace allpix {
    // type of the deposits
    class ChargeDeposit {
    public:
        ChargeDeposit(ROOT::Math::XYZVector position, double energy);

        ROOT::Math::XYZVector getPosition();
        double getEnergy();

    private:
        ROOT::Math::XYZVector position_;
        double energy_;
    };

    // message carrying all the deposits
    class DepositionMessage : public Message {
    public:
        DepositionMessage() : deposits() {}
        std::vector<ChargeDeposit>& getDeposits();

    private:
        std::vector<ChargeDeposit> deposits;
    };
}

#endif /* ALLPIX_MESSAGE_DEPOSITION_H */
