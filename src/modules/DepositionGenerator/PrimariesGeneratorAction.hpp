/**
 * @copyright Copyright (c) 2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_PRIMARIES_DEPOSITION_MODULE_GENERATOR_ACTION_H
#define ALLPIX_PRIMARIES_DEPOSITION_MODULE_GENERATOR_ACTION_H

#include "PrimariesReader.hpp"

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
    class PrimariesGeneratorAction : public G4VUserPrimaryGeneratorAction {
    public:
        /**
         * @brief Constructs the generator action
         * @param config Configuration of the \ref DepositionCosmicsModule module
         */
        explicit PrimariesGeneratorAction(const Configuration& config, std::shared_ptr<PrimariesReader> reader);

        /**
         * @brief Generate the particle for every event
         */
        void GeneratePrimaries(G4Event*) override;

    private:
        std::unique_ptr<G4ParticleGun> particle_gun_;
        const Configuration& config_;
        std::shared_ptr<PrimariesReader> reader_;
    };
} // namespace allpix

#endif /* ALLPIX_PRIMARIES_DEPOSITION_MODULE_GENERATOR_ACTION_H */
