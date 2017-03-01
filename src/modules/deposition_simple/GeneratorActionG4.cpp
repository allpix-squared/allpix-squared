/**
 *  @author John Idarraga <idarraga@cern.ch>
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "GeneratorActionG4.hpp"

#include "G4Event.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4GeneralParticleSource.hh"
#include "G4HEPEvtInterface.hh"

#include "G4RunManager.hh"

#include "CLHEP/Units/SystemOfUnits.h"
using namespace CLHEP;

using namespace allpix;

// construct and destruct the generator
GeneratorActionG4::GeneratorActionG4()
{
    
    G4int n_particle = 1;
    m_particleGun = std::make_unique<G4ParticleGun>(n_particle);
    
    //default kinematic
    //
    G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
    G4ParticleDefinition* particle = particleTable->FindParticle("e+");
    
    // FIXME: these parameters should not be fixed of course...
    m_particleGun->SetParticleDefinition(particle);
    m_particleGun->SetParticleTime(0.0*ns);
    m_particleGun->SetParticlePosition(G4ThreeVector(-25.*um, -25*um, 50*um));
    //m_particleGun->SetParticlePosition(G4ThreeVector(0.150*mm, 0.150*mm, 0.*mm));
    //m_particleGun->SetParticleMomentumDirection(G4ThreeVector(0.,0.,-1.));
    m_particleGun->SetParticleMomentumDirection(G4ThreeVector(0,0,-1));
    m_particleGun->SetParticleEnergy(500.0*keV);
    
    // store temporarily the position of incoming particles
    m_primaryParticlePos.clear();
    
}
GeneratorActionG4::~GeneratorActionG4() {}

// generate the particles
void GeneratorActionG4::GeneratePrimaries(G4Event* event) {
    m_particleGun->GeneratePrimaryVertex(event);
}
