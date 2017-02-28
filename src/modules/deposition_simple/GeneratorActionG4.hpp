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
#include "G4ThreeVector.hh"

class G4ParticleGun;
class G4Event;
class G4VPrimaryGenerator;

namespace allpix{
    /*enum SourceType {
     *    _ParticleGun = 0,
     *    _GeneralParticleSource,
     *    _HEPEvtInterface
    };*/
    
    class GeneratorActionG4 : public G4VUserPrimaryGeneratorAction {
    public:
        GeneratorActionG4();
        //AllPixPrimaryGeneratorAction(SourceType);
        ~GeneratorActionG4();
        
        // generate the primary events
        void GeneratePrimaries(G4Event*);
        
    private:
        // simple particule gun case
        std::unique_ptr<G4ParticleGun> m_particleGun;
        
        // store temporarily particle positions
        std::vector<G4ThreeVector> m_primaryParticlePos;
        
        //SourceType m_sType;
    };
}

#endif /*AllPixPrimaryGeneratorAction_h*/
