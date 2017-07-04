/**
 * @file
 * @brief Defines the particle generator
 * @copyright MIT License
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
    /**
     * @brief Generates the particles in every event
     */
    class GeneratorActionG4 : public G4VUserPrimaryGeneratorAction {
    public:
        /**
         * @brief Constructs the generator action
         * @param config Configuration of the \ref DepositionGeant4Module module
         */
        explicit GeneratorActionG4(const Configuration& config);

        /**
         * @brief Generate the particle for every event
         */
        void GeneratePrimaries(G4Event*) override;

    private:
        std::unique_ptr<G4GeneralParticleSource> particle_source_;
    };
} // namespace allpix

#endif /* ALLPIX_SIMPLE_DEPOSITION_MODULE_GENERATOR_ACTION_H */
