/**
 * @file
 * @brief Implementation of propagated charge object
 * @copyright MIT License
 */

#include "PropagatedCharge.hpp"

using namespace allpix;

PropagatedCharge::PropagatedCharge(ROOT::Math::XYZPoint local_position,
                                   ROOT::Math::XYZPoint global_position,
                                   unsigned int charge,
                                   double event_time,
                                   const DepositedCharge* deposited_charge)
    : SensorCharge(std::move(local_position), std::move(global_position), charge, event_time) {
    deposited_charge_ = const_cast<DepositedCharge*>(deposited_charge); // NOLINT
}

/**
 * Object is stored as TRef and can only be accessed if pointed object is in scope
 */
const DepositedCharge* PropagatedCharge::getDepositedCharge() const {
    return dynamic_cast<DepositedCharge*>(deposited_charge_.GetObject());
}

ClassImp(PropagatedCharge)
