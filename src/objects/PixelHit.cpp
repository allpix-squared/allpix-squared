/**
 * @file
 * @brief Implementation of object with digitized pixel hit
 * @copyright MIT License
 */

#include "PixelHit.hpp"

#include <set>

#include "DepositedCharge.hpp"
#include "PropagatedCharge.hpp"
#include "exceptions.h"

using namespace allpix;

PixelHit::PixelHit(Pixel pixel, double time, double signal, const PixelCharge* pixel_charge)
    : pixel_(std::move(pixel)), time_(time), signal_(signal) {
    pixel_charge_ = const_cast<PixelCharge*>(pixel_charge); // NOLINT
}

const Pixel& PixelHit::getPixel() const {
    return pixel_;
}

Pixel::Index PixelHit::getIndex() const {
    return getPixel().getIndex();
}

/**
 * @throws MissingReferenceException If the pointed object is not in scope
 *
 * Object is stored as TRef and can only be accessed if pointed object is in scope
 */
const PixelCharge* PixelHit::getPixelCharge() const {
    auto pixel_charge = dynamic_cast<PixelCharge*>(pixel_charge_.GetObject());
    if(pixel_charge == nullptr) {
        throw MissingReferenceException(typeid(*this), typeid(PixelCharge));
    }
    return pixel_charge;
}

/**
 * @throws MissingReferenceException If some object in the required history is not in scope
 *
 * MCParticles can only be fetched if the full history of objects are in scope and stored
 */
std::vector<const MCParticle*> PixelHit::getMCParticles() const {
    auto pixel_charge = getPixelCharge();

    // Find particles corresponding to every propagated charge
    auto propagated_charges = pixel_charge->getPropagatedCharges();
    std::set<const MCParticle*> unique_particles;
    for(auto& propagated_charge : propagated_charges) {
        auto deposited_charge = propagated_charge->getDepositedCharge();
        auto mc_particle = deposited_charge->getMCParticle();
        // NOTE if any deposited charge has no related mc particle this will fail
        unique_particles.insert(mc_particle);
    }

    // Return as a vector of mc particles
    return std::vector<const MCParticle*>(unique_particles.begin(), unique_particles.end());
}

ClassImp(PixelHit)
