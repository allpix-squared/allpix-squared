/**
 *  @author John Idarraga <idarraga@cern.ch>
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_SIMPLE_DEPOSITION_MODULE_GENERATOR_ACTION_H
#define ALLPIX_SIMPLE_DEPOSITION_MODULE_GENERATOR_ACTION_H

#include <memory>

#include "G4GeneralParticleSource.hh"
#include "G4ParticleDefinition.hh"
#include "G4ThreeVector.hh"
#include "G4VPrimaryGenerator.hh"
#include "G4VUserPrimaryGeneratorAction.hh"

class G4ParticleGun;

namespace allpix {

    class GeneratorActionG4 : public G4VUserPrimaryGeneratorAction {
    public:
        GeneratorActionG4(
            int n_particle, G4ParticleDefinition* particle, G4ThreeVector position, G4ThreeVector momentum, double energy);
        ~GeneratorActionG4() override;

        // generate the primary events
        void GeneratePrimaries(G4Event*) override;

    private:
        // simple particule gun case
        std::unique_ptr<G4ParticleGun> particleGun_;
    };
} // namespace allpix

#endif /* ALLPIX_SIMPLE_DEPOSITION_MODULE_GENERATOR_ACTION_H */
