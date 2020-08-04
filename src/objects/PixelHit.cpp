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
#include "exceptions.h"

using namespace allpix;

PixelHit::PixelHit(Pixel pixel, double time, double signal, const PixelCharge* pixel_charge)
    : pixel_(std::move(pixel)), time_(time), signal_(signal) {
    pixel_charge_ref_ = reinterpret_cast<uintptr_t>(pixel_charge); // NOLINT
    // Get the unique set of MC particles
    std::set<uintptr_t> unique_particles;
    for(auto& mc_particle : pixel_charge->mc_particles_ref_) {
        unique_particles.insert(mc_particle);
    }
    // Store the MC particle references
    for(auto& mc_particle : unique_particles) {
        mc_particles_ref_.push_back(mc_particle);
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
    auto pixel_charge = reinterpret_cast<PixelCharge*>(pixel_charge_ref_);
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
    for(auto& mc_particle_ptr : mc_particles_ref_) {
        auto mc_particle = reinterpret_cast<MCParticle*>(mc_particle_ptr);
        if(mc_particle == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(MCParticle));
        }
        mc_particles.emplace_back(dynamic_cast<MCParticle*>(mc_particle));
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
    for(auto& mc_particle_ptr : mc_particles_ref_) {
        auto mc_particle = reinterpret_cast<MCParticle*>(mc_particle_ptr);
        if(mc_particle == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(MCParticle));
        }
        auto particle = dynamic_cast<MCParticle*>(mc_particle);

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

void PixelHit::storeHistory() {
    pixel_charge_ = TRef(reinterpret_cast<PixelCharge*>(pixel_charge_ref_));
    pixel_charge_ref_ = 0;

    for(auto& mc_particle : mc_particles_ref_) {
        mc_particles_.push_back(reinterpret_cast<MCParticle*>(mc_particle));
        mc_particle = 0;
    }
}

void PixelHit::loadHistory() {
    pixel_charge_ref_ = reinterpret_cast<uintptr_t>(pixel_charge_.GetObject());

    for(auto& mc_particle : mc_particles_) {
        mc_particles_ref_.push_back(reinterpret_cast<uintptr_t>(mc_particle.GetObject()));
    }

    // FIXME we need to reset TRef member
}
