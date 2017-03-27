/**
 *  @author John Idarraga <idarraga@cern.ch>
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "GeneratorActionG4.hpp"

#include "G4Event.hh"
#include "G4GeneralParticleSource.hh"
#include "G4HEPEvtInterface.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"

#include "G4RunManager.hh"

using namespace allpix;

// construct and destruct the generator
GeneratorActionG4::GeneratorActionG4(
    int n_particle, G4ParticleDefinition* particle, G4ThreeVector position, G4ThreeVector direction, double energy)
    : particleGun_(std::make_unique<G4ParticleGun>(n_particle)) {
    particleGun_->SetParticleDefinition(particle);
    particleGun_->SetParticleTime(0.0); // FIXME: what is this time
    particleGun_->SetParticlePosition(position);
    particleGun_->SetParticleMomentumDirection(direction);
    particleGun_->SetParticleEnergy(energy);
}
GeneratorActionG4::~GeneratorActionG4() = default;

// generate the particles
void GeneratorActionG4::GeneratePrimaries(G4Event* event) {
    particleGun_->GeneratePrimaryVertex(event);
}
