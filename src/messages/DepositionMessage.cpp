/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "DepositionMessage.hpp"

using namespace allpix;

ChargeDeposit::ChargeDeposit(TVector3 position, double energy): position_(position), energy_(energy) {}

TVector3 ChargeDeposit::getPosition() {
    return position_;
}
double ChargeDeposit::getEnergy() {
    return energy_;
}
    
std::vector<ChargeDeposit> &DepositionMessage::getDeposits(){
    return deposits;
}
