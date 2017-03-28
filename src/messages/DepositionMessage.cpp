/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "DepositionMessage.hpp"

using namespace allpix;

ChargeDeposit::ChargeDeposit(ROOT::Math::XYZVector position, double energy)
    : position_(std::move(position)), energy_(energy) {}

ROOT::Math::XYZVector ChargeDeposit::getPosition() {
    return position_;
}
double ChargeDeposit::getEnergy() {
    return energy_;
}

std::vector<ChargeDeposit>& DepositionMessage::getDeposits() {
    return deposits;
}
