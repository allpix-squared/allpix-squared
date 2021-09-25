/**
 * @file
 * @brief Defines the CRY interface to Geant4
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_COSMICS_DEPOSITION_MODULE_GENERATOR_ACTION_H
#define ALLPIX_COSMICS_DEPOSITION_MODULE_GENERATOR_ACTION_H

#include <memory>

#include <G4DataVector.hh>
#include <G4ParticleGun.hh>
#include <G4ThreeVector.hh>
#include <G4VUserPrimaryGeneratorAction.hh>

#include "cry/CRYGenerator.h"
#include "cry/CRYParticle.h"
#include "cry/CRYSetup.h"
#include "cry/CRYUtils.h"

#include "core/config/Configuration.hpp"

namespace allpix {
    /**
     * @brief Generates the particles in every event
     */
    class CosmicsGeneratorActionG4 : public G4VUserPrimaryGeneratorAction {
    public:
        /**
         * @brief Constructs the generator action
         * @param config Configuration of the \ref DepositionCosmicsModule module
         */
        explicit CosmicsGeneratorActionG4(const Configuration& config);

        /**
         * @brief Generate the particle for every event
         */
        void GeneratePrimaries(G4Event*) override;

    private:
        std::unique_ptr<G4ParticleGun> particle_gun_;
        std::unique_ptr<CRYGenerator> cry_generator_;

        bool reset_particle_time_{};
        const Configuration& config_;
    };

    class GeneratorActionInitializationMaster {
    public:
        explicit GeneratorActionInitializationMaster(const Configuration&){};
    };
} // namespace allpix

#endif /* ALLPIX_COSMICS_DEPOSITION_MODULE_GENERATOR_ACTION_H */
