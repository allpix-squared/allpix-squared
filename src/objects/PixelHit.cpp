/**
 * @file
 * @brief Implementation of object with digitized pixel hit
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "PixelHit.hpp"

#include <set>

#include "DepositedCharge.hpp"
#include "PropagatedCharge.hpp"
#include "objects/exceptions.h"

using namespace allpix;

PixelHit::PixelHit(Pixel pixel, double time, double signal, const PixelCharge* pixel_charge)
    : pixel_(std::move(pixel)), time_(time), signal_(signal) {
    pixel_charge_ = const_cast<PixelCharge*>(pixel_charge); // NOLINT
    // Get the unique set of MC particles
    std::set<TRef> unique_particles;
    for(const auto& mc_particle : pixel_charge->mc_particles_) {
        unique_particles.insert(mc_particle);
    }
    // Store the MC particle references
    for(const auto& mc_particle : unique_particles) {
        mc_particles_.push_back(mc_particle);
    }
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
    auto* pixel_charge = dynamic_cast<PixelCharge*>(pixel_charge_.GetObject());
    if(pixel_charge == nullptr) {
        throw MissingReferenceException(typeid(*this), typeid(PixelCharge));
    }
    return pixel_charge;
}

/**
 * @throws MissingReferenceException If the pointed object is not in scope
 *
 * MCParticles can only be fetched if the full history of objects are in scope and stored
 */
std::vector<const MCParticle*> PixelHit::getMCParticles() const {

    std::vector<const MCParticle*> mc_particles;
    for(const auto& mc_particle : mc_particles_) {
        if(!mc_particle.IsValid() || mc_particle.GetObject() == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(MCParticle));
        }
        mc_particles.emplace_back(dynamic_cast<MCParticle*>(mc_particle.GetObject()));
    }

    // Return as a vector of mc particles
    return mc_particles;
}

/**
 * @throws MissingReferenceException If the pointed object is not in scope
 *
 * MCParticles can only be fetched if the full history of objects are in scope and stored
 */
std::vector<const MCParticle*> PixelHit::getPrimaryMCParticles() const {
    std::vector<const MCParticle*> primary_particles;
    for(const auto& mc_particle : mc_particles_) {
        if(!mc_particle.IsValid() || mc_particle.GetObject() == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(MCParticle));
        }
        auto* particle = dynamic_cast<MCParticle*>(mc_particle.GetObject());

        // Check for possible parents:
        if(particle->getParent() != nullptr) {
            continue;
        }
        primary_particles.emplace_back(particle);
    }

    // Return as a vector of mc particles
    return primary_particles;
}

void PixelHit::print(std::ostream& out) const {
    out << "PixelHit " << this->getIndex().X() << ", " << this->getIndex().Y() << ", " << this->getSignal() << ", "
        << this->getTime();
}
