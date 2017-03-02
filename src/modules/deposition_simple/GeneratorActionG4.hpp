/**
 *  @author John Idarraga <idarraga@cern.ch>
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_SIMPLE_DEPOSITION_MODULE_GENERATOR_ACTION_H
#define ALLPIX_SIMPLE_DEPOSITION_MODULE_GENERATOR_ACTION_H

#include <vector>

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4VPrimaryGenerator.hh"
#include "G4GeneralParticleSource.hh"
#include "G4ParticleDefinition.hh"
#include "G4ThreeVector.hh"

class G4ParticleGun;

namespace allpix{
    
    class GeneratorActionG4 : public G4VUserPrimaryGeneratorAction {
    public:
        GeneratorActionG4(int n_particle, G4ParticleDefinition *particle, G4ThreeVector position, G4ThreeVector momentum, double energy);
        ~GeneratorActionG4();
        
        // generate the primary events
        void GeneratePrimaries(G4Event*);
        
    private:
        // simple particule gun case
        std::unique_ptr<G4ParticleGun> particleGun_;
    };
}

#endif /*AllPixPrimaryGeneratorAction_h*/
