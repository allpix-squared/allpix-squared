/**
 * @file
 * @brief Implementation of object with set of particles at pixel
 *
 * @copyright Copyright (c) 2017-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "PixelCharge.hpp"

#include <set>
#include "objects/exceptions.h"

using namespace allpix;

PixelCharge::PixelCharge(Pixel pixel, long charge, const std::vector<const PropagatedCharge*>& propagated_charges)
    : pixel_(std::move(pixel)), charge_(charge) {
    // Unique set of MC particles
    std::set<const MCParticle*> unique_particles;
    // Store all propagated charges and their MC particles
    for(const auto& propagated_charge : propagated_charges) {
        propagated_charges_.emplace_back(propagated_charge);
        unique_particles.insert(propagated_charge->mc_particle_.get());
    }
    // Store the MC particle references
    for(const auto& mc_particle : unique_particles) {
        // Local and global time are set as the earliest time found among the MCParticles:
        if(mc_particle != nullptr) {
            const auto* primary = mc_particle->getPrimary();
            local_time_ = std::min(local_time_, primary->getLocalTime());
            global_time_ = std::min(global_time_, primary->getGlobalTime());
        }
        mc_particles_.emplace_back(mc_particle);
    }

    // If no appropriate reference time has been found, set them to zero:
    if(local_time_ > std::numeric_limits<double>::max()) {
        local_time_ = 0.;
    }
    if(global_time_ > std::numeric_limits<double>::max()) {
        global_time_ = 0.;
    }

    // No pulse provided, set full charge in first bin:
    pulse_.addCharge(static_cast<double>(charge), 0);
}

// WARNING PixelCharge always returns a positive "collected" charge...
PixelCharge::PixelCharge(Pixel pixel, Pulse pulse, const std::vector<const PropagatedCharge*>& propagated_charges)
    : PixelCharge(std::move(pixel), static_cast<long>(pulse.getCharge()), propagated_charges) {
    pulse_ = std::move(pulse); // NOLINT
}

const Pixel& PixelCharge::getPixel() const { return pixel_; }

Pixel::Index PixelCharge::getIndex() const { return getPixel().getIndex(); }

long PixelCharge::getCharge() const { return charge_; }

unsigned long PixelCharge::getAbsoluteCharge() const { return static_cast<unsigned long>(std::abs(charge_)); }

const Pulse& PixelCharge::getPulse() const { return pulse_; }

double PixelCharge::getGlobalTime() const { return global_time_; }

double PixelCharge::getLocalTime() const { return local_time_; }

/**
 * @throws MissingReferenceException If the pointed object is not in scope
 *
 * Objects are stored as vector of TRef and can only be accessed if pointed objects are in scope
 */
std::vector<const PropagatedCharge*> PixelCharge::getPropagatedCharges() const {
    // FIXME: This is not very efficient unfortunately
    std::vector<const PropagatedCharge*> propagated_charges;
    for(const auto& propagated_charge : propagated_charges_) {
        if(propagated_charge.get() == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(PropagatedCharge));
        }
        propagated_charges.emplace_back(propagated_charge.get());
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
std::vector<const MCParticle*> PixelCharge::getPrimaryMCParticles() const {
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
        << global_center_location.Z() << ") mm\n"
        << "Local time:" << local_time_ << " ns\n"
        << "Global time:" << global_time_ << " ns\n";
}

void PixelCharge::loadHistory() {
    std::for_each(propagated_charges_.begin(), propagated_charges_.end(), [](auto& n) { n.get(); });
    std::for_each(mc_particles_.begin(), mc_particles_.end(), [](auto& n) { n.get(); });
}
void PixelCharge::petrifyHistory() {
    std::for_each(propagated_charges_.begin(), propagated_charges_.end(), [](auto& n) { n.store(); });
    std::for_each(mc_particles_.begin(), mc_particles_.end(), [](auto& n) { n.store(); });
}
