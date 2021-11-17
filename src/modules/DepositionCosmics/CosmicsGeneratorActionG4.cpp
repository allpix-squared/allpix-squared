/**
 * @file
 * @brief Implements the particle generator
 * @remark Based on code from John Idarraga
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "CosmicsGeneratorActionG4.hpp"

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
    auto setup = new CRYSetup(config_.get<std::string>("_cry_config"), config_.get<std::string>("data_path"));
    cry_generator_ = std::make_unique<CRYGenerator>(setup);
}

/**
 * Called automatically for every event
 */
void CosmicsGeneratorActionG4::GeneratePrimaries(G4Event* event) {

    // Let CRY generate the particles
    std::vector<CRYParticle*> vect;
    cry_generator_->genEvent(&vect);

    LOG(DEBUG) << "CRY generated " << vect.size() << " particles";

    for(const auto& particle : vect) {

        LOG(DEBUG) << "  " << CRYUtils::partName(particle->id()) << ": charge=" << particle->charge() << " "
                   << std::setprecision(4) << "energy=" << Units::display(particle->ke(), {"MeV", "GeV"}) << " "
                   << "pos="
                   << Units::display(G4ThreeVector(particle->x() * 1e3, particle->y() * 1e3, particle->z() * 1e3), {"m"})
                   << " "
                   << "direction cosines " << G4ThreeVector(particle->u(), particle->v(), particle->w());

        auto* pdg_table = G4ParticleTable::GetParticleTable();
        particle_gun_->SetParticleDefinition(pdg_table->FindParticle(particle->PDGid()));
        particle_gun_->SetParticleEnergy(particle->ke() * CLHEP::MeV);
        particle_gun_->SetParticlePosition(
            G4ThreeVector(particle->x() * CLHEP::m, particle->y() * CLHEP::m, particle->z() * CLHEP::m));
        particle_gun_->SetParticleMomentumDirection(G4ThreeVector(particle->u(), particle->v(), particle->w()));
        particle_gun_->SetParticleTime(particle->t());
        particle_gun_->GeneratePrimaryVertex(event);
    }
}
