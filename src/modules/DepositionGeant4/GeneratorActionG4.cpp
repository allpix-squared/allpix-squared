/**
 *  @author John Idarraga <idarraga@cern.ch>
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "GeneratorActionG4.hpp"

#include <limits>
#include <memory>

#include <G4Event.hh>
#include <G4GeneralParticleSource.hh>
#include <G4ParticleDefinition.hh>
#include <G4ParticleTable.hh>

#include "core/config/InvalidValueError.hpp"
#include "core/utils/log.h"
#include "tools/geant4.h"

using namespace allpix;

// construct and destruct the generator
GeneratorActionG4::GeneratorActionG4(Configuration config) : particle_source_(std::make_unique<G4GeneralParticleSource>()) {
    // set verbosity to zero
    particle_source_->SetVerbosity(0);

    // get source specific parameters
    auto single_source = particle_source_->GetCurrentSource();

    // find particle
    G4ParticleDefinition* particle =
        G4ParticleTable::GetParticleTable()->FindParticle(config.get<std::string>("particle_type"));
    if(particle == nullptr) {
        // FIXME: more information about available particle
        throw InvalidValueError(config, "particle_type", "particle type does not exist");
    }

    // set global parameters
    single_source->SetNumberOfParticles(static_cast<int>(config.get<unsigned int>("particle_amount")));
    single_source->SetParticleDefinition(particle);
    single_source->SetParticleTime(0.0); // FIXME: what is this time

    // set position parameters
    single_source->GetPosDist()->SetPosDisType("Point");
    single_source->GetPosDist()->SetCentreCoords(config.get<G4ThreeVector>("particle_position"));

    // set distribution parameters
    single_source->GetAngDist()->SetAngDistType("planar");
    G4ThreeVector direction = config.get<G4ThreeVector>("particle_direction");
    if(fabs(direction.mag() - 1.0) > std::numeric_limits<double>::epsilon()) {
        LOG(WARNING) << "Momentum direction is not a unit vector: any magnitude is ignored";
    }
    single_source->GetAngDist()->SetParticleMomentumDirection(direction);

    // set energy parameters
    single_source->GetEneDist()->SetEnergyDisType("Mono");
    single_source->GetEneDist()->SetMonoEnergy(config.get<double>("particle_energy"));
}
GeneratorActionG4::~GeneratorActionG4() = default;

// generate the particles
void GeneratorActionG4::GeneratePrimaries(G4Event* event) {
    particle_source_->GeneratePrimaryVertex(event);
}
