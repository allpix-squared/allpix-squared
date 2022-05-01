/**
 * @file
 * @brief Defines the CRY interface to Geant4
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_DEPOSITION_MODULE_GENERATOR_ACTION_GENIE_H
#define ALLPIX_DEPOSITION_MODULE_GENERATOR_ACTION_GENIE_H

#include <memory>

#include <G4DataVector.hh>
#include <G4ParticleGun.hh>
#include <G4ThreeVector.hh>
#include <G4VUserPrimaryGeneratorAction.hh>

#include "core/config/Configuration.hpp"

namespace allpix {
    /**
     * @brief Generates the particles in every event
     */
    class PrimariesReader : public G4VUserPrimaryGeneratorAction {
    public:
        /**
         * @brief Constructs the generator action
         * @param config Configuration of the \ref DepositionCosmicsModule module
         */
        explicit GeneratorActionGenie(const Configuration& config);

        /**
         * @brief Generate the particle for every event
         */
        void GeneratePrimaries(G4Event*) override;

    private:
        std::unique_ptr<G4ParticleGun> particle_gun_;
        const Configuration& config_;
    };
} // namespace allpix

#endif /* ALLPIX_DEPOSITION_MODULE_GENERATOR_ACTION_GENIE_H */
