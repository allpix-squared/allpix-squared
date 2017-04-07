/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "ChargeDeposit.hpp"

#include <utility>

using namespace allpix;

ChargeDeposit::ChargeDeposit(ROOT::Math::XYZVector position, unsigned int charge) : position_(position), charge_(charge) {}

ROOT::Math::XYZPoint ChargeDeposit::getPosition() const {
    return position_;
}

unsigned int ChargeDeposit::getCharge() const {
    return charge_;
}
