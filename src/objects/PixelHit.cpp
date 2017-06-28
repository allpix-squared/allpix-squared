/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "PixelHit.hpp"

#include <set>

#include "DepositedCharge.hpp"
#include "PropagatedCharge.hpp"

using namespace allpix;

PixelHit::PixelHit(Pixel pixel, double time, double signal, const PixelCharge* pixel_charge)
    : pixel_(std::move(pixel)), time_(time), signal_(signal) {
    pixel_charge_ = const_cast<PixelCharge*>(pixel_charge);
}

PixelHit::Pixel PixelHit::getPixel() const {
    return pixel_;
}

const PixelCharge* PixelHit::getPixelCharge() const {
    return dynamic_cast<PixelCharge*>(pixel_charge_.GetObject());
}

std::vector<const MCParticle*> PixelHit::getMCParticles() const {
    auto pixel_charge = getPixelCharge();
    if(pixel_charge == nullptr) {
        return std::vector<const MCParticle*>();
    }

    // Find particles corresponding to every propagated charge
    auto propagated_charges = pixel_charge->getPropagatedCharges();
    std::set<const MCParticle*> unique_particles;
    for(auto& propagated_charge : propagated_charges) {
        if(propagated_charge != nullptr) {
            auto deposited_charge = propagated_charge->getDepositedCharge();
            if(deposited_charge != nullptr) {
                auto mc_particle = deposited_charge->getMCParticle();
                // FIXME can also save nullptr as an `unique particle`
                unique_particles.insert(mc_particle);
            }
        }
    }

    // Return as a vector of mc particles
    return std::vector<const MCParticle*>(unique_particles.begin(), unique_particles.end());
}

ClassImp(PixelHit)
