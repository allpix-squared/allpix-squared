/**
 *  @author John Idarraga <idarraga@cern.ch>
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_SIMPLE_DEPOSITION_MODULE_GENERATOR_ACTION_H
#define ALLPIX_SIMPLE_DEPOSITION_MODULE_GENERATOR_ACTION_H

#include <memory>

#include <G4GeneralParticleSource.hh>
#include <G4ParticleDefinition.hh>
#include <G4SDManager.hh>
#include <G4ThreeVector.hh>
#include <G4VUserPrimaryGeneratorAction.hh>

#include "core/config/Configuration.hpp"

namespace allpix {

    class GeneratorActionG4 : public G4VUserPrimaryGeneratorAction {
    public:
        explicit GeneratorActionG4(const Configuration& config);
        ~GeneratorActionG4() override;

        // generate the primary events
        void GeneratePrimaries(G4Event*) override;

    private:
        // simple particule gun case
        std::unique_ptr<G4GeneralParticleSource> particle_source_;
    };
} // namespace allpix

#endif /* ALLPIX_SIMPLE_DEPOSITION_MODULE_GENERATOR_ACTION_H */
