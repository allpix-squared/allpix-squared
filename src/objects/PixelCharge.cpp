/**
 * @file
 * @brief Implementation of object with set of particles at pixel
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "PixelCharge.hpp"

#include "exceptions.h"

using namespace allpix;

PixelCharge::PixelCharge(Pixel pixel, unsigned int charge, std::vector<const PropagatedCharge*> propagated_charges)
    : pixel_(std::move(pixel)), charge_(charge) {
    for(auto& propagated_charge : propagated_charges) {
        propagated_charges_.Add(const_cast<PropagatedCharge*>(propagated_charge)); // NOLINT
    }
}

const Pixel& PixelCharge::getPixel() const {
    return pixel_;
}

Pixel::Index PixelCharge::getIndex() const {
    return getPixel().getIndex();
}

unsigned int PixelCharge::getCharge() const {
    return charge_;
}

/**
 * @throws MissingReferenceException If the pointed object is not in scope
 *
 * Objects are stored as TRefArray and can only be accessed if pointed objects are in scope
 */
std::vector<const PropagatedCharge*> PixelCharge::getPropagatedCharges() const {
    // FIXME: This is not very efficient unfortunately
    std::vector<const PropagatedCharge*> propagated_charges;
    for(int i = 0; i < propagated_charges_.GetEntries(); ++i) {
        propagated_charges.emplace_back(dynamic_cast<PropagatedCharge*>(propagated_charges_[i]));
        if(propagated_charges.back() == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(PropagatedCharge));
        }
    }
    return propagated_charges;
}

ClassImp(PixelCharge)
