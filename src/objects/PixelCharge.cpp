/**
 * @file
 * @brief Implementation of object with set of particles at pixel
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "PixelCharge.hpp"

#include <set>
#include "exceptions.h"

using namespace allpix;

PixelCharge::PixelCharge(Pixel pixel, unsigned int charge, const std::vector<const PropagatedCharge*>& propagated_charges)
    : pixel_(std::move(pixel)), charge_(charge) {
    // Unique set of MC particles
    std::set<Object*> unique_particles;
    // Store all propagated charges and their MC particles
    for(auto& propagated_charge : propagated_charges) {
        propagated_charges_ref_.push_back(const_cast<PropagatedCharge*>(propagated_charge)); // NOLINT
        unique_particles.insert(propagated_charge->mc_particle_ref_);
    }
    // Store the MC particle references
    for(auto& mc_particle : unique_particles) {
        mc_particles_ref_.push_back(mc_particle);
    }

    // No pulse provided, set full charge in first bin:
    pulse_.addCharge(charge, 0);
}

// WARNING PixelCharge always returns a positive "collected" charge...
PixelCharge::PixelCharge(Pixel pixel, Pulse pulse, const std::vector<const PropagatedCharge*>& propagated_charges)
    : PixelCharge(std::move(pixel), static_cast<unsigned int>(std::abs(pulse.getCharge())), propagated_charges) {
    pulse_ = std::move(pulse);
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

const Pulse& PixelCharge::getPulse() const {
    return pulse_;
}

/**
 * @throws MissingReferenceException If the pointed object is not in scope
 *
 * Objects are stored as vector of TRef and can only be accessed if pointed objects are in scope
 */
std::vector<const PropagatedCharge*> PixelCharge::getPropagatedCharges() const {
    // FIXME: This is not very efficient unfortunately
    std::vector<const PropagatedCharge*> propagated_charges;
    for(auto& propagated_charge : propagated_charges_ref_) {
        if(propagated_charge == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(PropagatedCharge));
        }
        propagated_charges.emplace_back(dynamic_cast<PropagatedCharge*>(propagated_charge));
    }
    return propagated_charges;
}

/**
 * @throws MissingReferenceException If the pointed object is not in scope
 *
 * MCParticles can only be fetched if the full history of objects are in scope and stored
 */
std::vector<const MCParticle*> PixelCharge::getMCParticles() const {

    std::vector<const MCParticle*> mc_particles;
    for(auto& mc_particle : mc_particles_ref_) {
        if(mc_particle == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(MCParticle));
        }
        mc_particles.emplace_back(dynamic_cast<MCParticle*>(mc_particle));
    }

    // Return as a vector of mc particles
    return mc_particles;
}

void PixelCharge::print(std::ostream& out) const {
    auto local_center_location = pixel_.getLocalCenter();
    auto global_center_location = pixel_.getGlobalCenter();
    auto pixel_index = pixel_.getIndex();

    out << "--- Pixel charge information\n";
    out << "Pixel: (" << pixel_index.X() << ", " << pixel_index.Y() << ")\n"
        << "Charge: " << charge_ << " e\n"
        << "Local Position: (" << local_center_location.X() << ", " << local_center_location.Y() << ", "
        << local_center_location.Z() << ") mm\n"
        << "Global Position: (" << global_center_location.X() << ", " << global_center_location.Y() << ", "
        << global_center_location.Z() << ") mm\n";
}

void PixelCharge::storeHistory() {
    for(auto& propagated_charge : propagated_charges_ref_) {
        propagated_charges_.push_back(propagated_charge);
    }

    for(auto& mc_particle : mc_particles_ref_) {
        mc_particles_.push_back(mc_particle);
    }
}

void PixelCharge::loadHistory() {
    for(auto& propagated_charge : propagated_charges_) {
        propagated_charges_ref_.push_back(dynamic_cast<Object*>(propagated_charge.GetObject()));
    }

    for(auto& mc_particle : mc_particles_) {
        mc_particles_ref_.push_back(dynamic_cast<Object*>(mc_particle.GetObject()));
    }

    // FIXME we need to reset TRef member
}
