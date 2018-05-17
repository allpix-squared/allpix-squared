/**
 * @file
 * @brief Implementation of pixel pulse object
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "PixelPulse.hpp"

#include "exceptions.h"

using namespace allpix;

PixelPulse::PixelPulse(Pixel pixel, double time_resolution, double time_total)
    : pixel_(std::move(pixel)), resolution_(time_resolution) {

    auto bins = static_cast<size_t>(time_total / time_resolution);
    pulse_.resize(bins, 0.);
}

void PixelPulse::addCurrent(CarrierType type, double time, double current, const DepositedCharge* deposited_charge) {
    auto bin = static_cast<size_t>(time / resolution_);

    // FIXME rethink how the current looks like depending on carrier type and drift direction
    if(type == CarrierType::ELECTRON) {
        pulse_.at(bin) -= current;
    } else {
        pulse_.at(bin) += current;
    }

    deposited_charges_.Add(const_cast<DepositedCharge*>(deposited_charge)); // NOLINT
}

/**
 * @throws MissingReferenceException If the pointed object is not in scope
 *
 * Objects are stored as TRefArray and can only be accessed if pointed objects are in scope
 */
std::vector<const DepositedCharge*> PixelPulse::getDepositedCharges() const {
    // FIXME: This is not very efficient unfortunately
    std::vector<const DepositedCharge*> deposited_charges;
    for(int i = 0; i < deposited_charges_.GetEntries(); ++i) {
        deposited_charges.emplace_back(dynamic_cast<DepositedCharge*>(deposited_charges_[i]));
        if(deposited_charges.back() == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(DepositedCharge));
        }
    }
    return deposited_charges;
}
