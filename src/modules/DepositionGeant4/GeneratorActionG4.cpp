/**
 * @file
 * @brief Implements the particle generator
 * @remark Based on code from John Idarraga
 * @copyright MIT License
 */

#include "GeneratorActionG4.hpp"

#include <limits>
#include <memory>

#include <G4Event.hh>
#include <G4GeneralParticleSource.hh>
#include <G4ParticleDefinition.hh>
#include <G4ParticleTable.hh>

#include "core/config/exceptions.h"
#include "core/utils/log.h"
#include "tools/geant4.h"

using namespace allpix;

GeneratorActionG4::GeneratorActionG4(const Configuration& config)
    : particle_source_(std::make_unique<G4GeneralParticleSource>()) {
    // Set verbosity of source to off
    particle_source_->SetVerbosity(0);

    // Get source specific parameters
    auto single_source = particle_source_->GetCurrentSource();

    // Find Geant4 particle
    G4ParticleDefinition* particle =
        G4ParticleTable::GetParticleTable()->FindParticle(config.get<std::string>("particle_type"));
    if(particle == nullptr) {
        // FIXME more information about available particle
        throw InvalidValueError(config, "particle_type", "particle type does not exist");
    }

    // Set global parameters of the source
    // FIXME keep number of particles always at one?
    single_source->SetNumberOfParticles(1);
    single_source->SetParticleDefinition(particle);
    // FIXME What is this time
    single_source->SetParticleTime(0.0);

    // Set position parameters
    single_source->GetPosDist()->SetPosDisType("Beam");
    single_source->GetPosDist()->SetBeamSigmaInR(config.get<double>("beam_size", 0));
    single_source->GetPosDist()->SetCentreCoords(config.get<G4ThreeVector>("beam_position"));

    // Set angle distribution parameters
    single_source->GetAngDist()->SetAngDistType("beam2d");
    single_source->GetAngDist()->DefineAngRefAxes("angref1", G4ThreeVector(-1., 0, 0));
    G4TwoVector divergence = config.get<G4TwoVector>("beam_divergence", G4TwoVector(0., 0.));
    single_source->GetAngDist()->SetBeamSigmaInAngX(divergence.x());
    single_source->GetAngDist()->SetBeamSigmaInAngY(divergence.y());
    G4ThreeVector direction = config.get<G4ThreeVector>("beam_direction");
    if(fabs(direction.mag() - 1.0) > std::numeric_limits<double>::epsilon()) {
        LOG(WARNING) << "Momentum direction is not a unit vector: magnitude is ignored";
    }
    single_source->GetAngDist()->SetParticleMomentumDirection(direction);

    // Set energy parameters
    single_source->GetEneDist()->SetEnergyDisType("Gauss");
    single_source->GetEneDist()->SetMonoEnergy(config.get<double>("beam_energy"));
    single_source->GetEneDist()->SetBeamSigmaInE(config.get<double>("beam_energy_spread", 0.));
}

/**
 * Called automatically for every event
 */
void GeneratorActionG4::GeneratePrimaries(G4Event* event) {
    particle_source_->GeneratePrimaryVertex(event);
}
