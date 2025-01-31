/**
 * @file
 * @brief Implements the interface of the Geant4 ParticleGun to CRY
 *
 * @copyright Copyright (c) 2021-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "CosmicsGeneratorActionG4.hpp"
#include "DepositionCosmicsModule.hpp"
#include "RNGWrapper.hpp"

#include <limits>
#include <memory>
#include <regex>

#include <G4Event.hh>
#include <G4ParticleTable.hh>

#include "core/config/exceptions.h"
#include "core/utils/log.h"
#include "tools/geant4/geant4.h"

using namespace allpix;

CosmicsGeneratorActionG4::CosmicsGeneratorActionG4(const Configuration& config)
    : particle_gun_(std::make_unique<G4ParticleGun>()), config_(config) {

    LOG(DEBUG) << "Setting up CRY generator";
    LOG(DEBUG) << "CRY configuration: " << config_.get<std::string>("_cry_config");
    LOG(DEBUG) << "CRY data: " << config_.get<std::string>("data_path");
    auto* setup = new CRYSetup(config_.get<std::string>("_cry_config"), config_.get<std::string>("data_path"));
    cry_generator_ = std::make_unique<CRYGenerator>(setup);

    // Set up random number generator to use the Geant4-internal one which is seeded per-event:
    LOG(DEBUG) << "Configuring CRY random engine to use Geant4's event-seeded engine";
    RNGWrapper<CLHEP::HepRandomEngine>::set(CLHEP::HepRandom::getTheEngine(), &CLHEP::HepRandomEngine::flat);
    setup->setRandomFunction(RNGWrapper<CLHEP::HepRandomEngine>::rng);

    // Parse other configuration parameters:
    reset_particle_time_ = config_.get<bool>("reset_particle_time");
}

/**
 * Called automatically for every event
 */
void CosmicsGeneratorActionG4::GeneratePrimaries(G4Event* event) {

    // Let CRY generate the particles
    std::vector<CRYParticle*> vect;
    LOG(DEBUG) << "Absolute time simulated before shower: "
               << Units::display(Units::get(cry_generator_->timeSimulated(), "s"), {"ns", "us", "ms"});
    cry_generator_->genEvent(&vect);
    LOG(DEBUG) << "CRY generated " << vect.size() << " particles";
    LOG(INFO) << "Absolute time simulated by CRY after shower: "
              << Units::display(Units::get(cry_generator_->timeSimulated(), "s"), {"ns", "us", "ms"});

    // Update simulation time in the framework base units
    DepositionCosmicsModule::cry_instance_time_simulated_ = cry_generator_->timeSimulated() * 1e9;

    // Event time frame starts with first particle arriving
    double event_starting_time = std::numeric_limits<double>::max();
    if(!reset_particle_time_) {
        for(const auto& particle : vect) {
            event_starting_time = std::min(event_starting_time, particle->t());
        }
    }

    for(const auto& particle : vect) {
        auto* pdg_table = G4ParticleTable::GetParticleTable();
        particle_gun_->SetParticleDefinition(pdg_table->FindParticle(particle->PDGid()));
        particle_gun_->SetParticleEnergy(particle->ke() * CLHEP::MeV);
        particle_gun_->SetParticlePosition(
            G4ThreeVector(particle->x() * CLHEP::m, particle->y() * CLHEP::m, particle->z() * CLHEP::m));
        particle_gun_->SetParticleMomentumDirection(G4ThreeVector(particle->u(), particle->v(), particle->w()));

        double time = (reset_particle_time_ ? 0. : particle->t() - event_starting_time);
        particle_gun_->SetParticleTime(time);
        particle_gun_->GeneratePrimaryVertex(event);

        LOG(DEBUG) << "  " << CRYUtils::partName(particle->id()) << ": charge=" << particle->charge() << std::setprecision(4)
                   << " energy=" << Units::display(particle->ke(), {"MeV", "GeV"}) << " pos="
                   << Units::display(G4ThreeVector(particle->x() * 1e3, particle->y() * 1e3, particle->z() * 1e3), {"m"})
                   << " dir. cos=" << G4ThreeVector(particle->u(), particle->v(), particle->w())
                   << " t=" << Units::display(Units::get(time, "s"), {"ns", "us", "ms"});
    }
}
