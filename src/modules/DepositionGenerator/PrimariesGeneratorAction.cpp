/**
 * @file
 * @brief Implements the interface to the Geant4 ParticleGun
 *
 * @copyright Copyright (c) 2022-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "PrimariesGeneratorAction.hpp"
#include "PrimariesReader.hpp"

#include <limits>
#include <memory>

#include <G4Event.hh>
#include <G4ParticleTable.hh>
#include <G4TransportationManager.hh>

#include "core/config/exceptions.h"
#include "core/utils/log.h"
#include "tools/geant4/geant4.h"

using namespace allpix;

PrimariesGeneratorAction::PrimariesGeneratorAction(const Configuration&, std::shared_ptr<PrimariesReader> reader)
    : particle_gun_(std::make_unique<G4ParticleGun>()), reader_(std::move(reader)) {

    LOG(DEBUG) << "Setting up Geant4 generator action";
}

bool PrimariesGeneratorAction::check_vertex_inside_world(const G4ThreeVector& pos) const {
    auto* solid = G4TransportationManager::GetTransportationManager()
                      ->GetNavigatorForTracking()
                      ->GetWorldVolume()
                      ->GetLogicalVolume()
                      ->GetSolid();
    return solid->Inside(pos) == kInside;
}

/**
 * Called automatically for every event
 */
void PrimariesGeneratorAction::GeneratePrimaries(G4Event* event) {

    // Read next set of primary particles from the data file
    using PrimaryParticle = PrimariesReader::Particle;
    std::vector<PrimaryParticle> particles = reader_->getParticles();

    // Dispatch them to the Geant4 particle gun
    LOG(DEBUG) << "Primary particles generated:";
    for(const auto& particle : particles) {
        // Check world boundary of primary vertex
        if(!check_vertex_inside_world(particle.position())) {
            LOG(WARNING) << "Vertex at " << particle.position() << " outside world volume, skipping.";
            continue;
        }

        auto* pdg_table = G4ParticleTable::GetParticleTable();
        particle_gun_->SetParticleDefinition(pdg_table->FindParticle(particle.pdg()));
        particle_gun_->SetParticleEnergy(particle.energy());
        particle_gun_->SetParticlePosition(particle.position());
        particle_gun_->SetParticleMomentumDirection(particle.direction());

        particle_gun_->SetParticleTime(particle.time());
        particle_gun_->GeneratePrimaryVertex(event);

        LOG(DEBUG) << " " << particle.pdg() << ":\t" << std::setprecision(4)
                   << " energy=" << Units::display(particle.energy(), {"MeV", "GeV"})
                   << " pos=" << Units::display(particle.position(), {"um", "mm", "cm"}) << " dir=" << particle.direction()
                   << " t=" << Units::display(particle.time(), {"ns", "us", "ms"});
    }
}
