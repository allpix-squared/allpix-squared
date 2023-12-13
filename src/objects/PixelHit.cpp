/**
 * @file
 * @brief Implementation of object with digitized pixel hit
 *
 * @copyright Copyright (c) 2017-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "PixelHit.hpp"

#include <set>

#include "DepositedCharge.hpp"
#include "PropagatedCharge.hpp"
#include "objects/exceptions.h"

using namespace allpix;

PixelHit::PixelHit(Pixel pixel,
                   double local_time,
                   double global_time,
                   double signal,
                   const PixelCharge* pixel_charge,
                   const PixelPulse* pixel_pulse)
    : pixel_(std::move(pixel)), local_time_(local_time), global_time_(global_time), signal_(signal) {

    pixel_pulse_ = PointerWrapper<PixelPulse>(pixel_pulse);
    pixel_charge_ = PointerWrapper<PixelCharge>(pixel_charge);
    if(pixel_charge != nullptr) {
        // Get the unique set of MC particles
        std::set<const MCParticle*> unique_particles;
        for(const auto& mc_particle : pixel_charge->mc_particles_) {
            unique_particles.insert(mc_particle.get());
        }
        // Store the MC particle references
        for(const auto& mc_particle : unique_particles) {
            mc_particles_.emplace_back(mc_particle);
        }
    }
}

const Pixel& PixelHit::getPixel() const { return pixel_; }

Pixel::Index PixelHit::getIndex() const { return getPixel().getIndex(); }

/**
 * @throws MissingReferenceException If the pointed object is not in scope
 *
 * Object is stored as TRef and can only be accessed if pointed object is in scope
 */
const PixelCharge* PixelHit::getPixelCharge() const {
    auto* pixel_charge = pixel_charge_.get();
    if(pixel_charge == nullptr) {
        throw MissingReferenceException(typeid(*this), typeid(PixelCharge));
    }
    return pixel_charge;
}

/**
 * @throws MissingReferenceException If the pointed object is not in scope
 *
 * Object is stored as TRef and can only be accessed if pointed object is in scope
 */
const PixelPulse* PixelHit::getPixelPulse() const {
    auto* pixel_pulse = pixel_pulse_.get();
    if(pixel_pulse == nullptr) {
        throw MissingReferenceException(typeid(*this), typeid(PixelPulse));
    }
    return pixel_pulse;
}

/**
 * @throws MissingReferenceException If the pointed object is not in scope
 *
 * MCParticles can only be fetched if the full history of objects are in scope and stored
 */
std::vector<const MCParticle*> PixelHit::getMCParticles() const {

    std::vector<const MCParticle*> mc_particles;
    for(const auto& mc_particle : mc_particles_) {
        if(mc_particle.get() == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(MCParticle));
        }
        mc_particles.emplace_back(mc_particle.get());
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
        auto* particle = mc_particle.get();
        if(particle == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(MCParticle));
        }

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
        << this->getLocalTime() << ", " << this->getGlobalTime() << ", " << this->getPixel().getGlobalCenter().X() << ", "
        << this->getPixel().getGlobalCenter().Y() << ", " << this->getPixel().getGlobalCenter().Z();
}

void PixelHit::loadHistory() {
    pixel_charge_.get();
    std::for_each(mc_particles_.begin(), mc_particles_.end(), [](auto& n) { n.get(); });
}
void PixelHit::petrifyHistory() {
    pixel_charge_.store();
    std::for_each(mc_particles_.begin(), mc_particles_.end(), [](auto& n) { n.store(); });
}
