/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "PixelCharge.hpp"

using namespace allpix;

PixelCharge::PixelCharge(Pixel pixel, unsigned int charge, std::vector<const PropagatedCharge*> propagated_charges)
    : pixel_(std::move(pixel)), charge_(charge) {
    for(auto& propagated_charge : propagated_charges) {
        // FIXME: This const cast has to be done
        propagated_charges_.Add(const_cast<PropagatedCharge*>(propagated_charge)); // NOLINT
    }
}

PixelCharge::Pixel PixelCharge::getPixel() const {
    return pixel_;
}

unsigned int PixelCharge::getCharge() const {
    return charge_;
}

std::vector<const PropagatedCharge*> PixelCharge::getPropagatedCharges() const {
    // FIXME: This is not very efficient unfortunately
    std::vector<const PropagatedCharge*> propagated_charges;
    for(int i = 0; i < propagated_charges_.GetEntries(); ++i) {
        propagated_charges.emplace_back(dynamic_cast<PropagatedCharge*>(propagated_charges_[i]));
    }
    return propagated_charges;
}

ClassImp(PixelCharge)
