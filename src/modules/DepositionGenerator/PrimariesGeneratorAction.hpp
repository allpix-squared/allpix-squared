/**
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_PRIMARIES_DEPOSITION_MODULE_GENERATOR_ACTION_H
#define ALLPIX_PRIMARIES_DEPOSITION_MODULE_GENERATOR_ACTION_H

#include <memory>

#include <G4DataVector.hh>
#include <G4ParticleGun.hh>
#include <G4ThreeVector.hh>
#include <G4VUserPrimaryGeneratorAction.hh>

#include "core/config/Configuration.hpp"

namespace allpix {
    class PrimariesReader;

    /**
     * @brief Generates the particles in every event
     */
    class PrimariesGeneratorAction : public G4VUserPrimaryGeneratorAction {
    public:
        /**
         * @brief Constructs the generator action
         * @param config Configuration of the \ref DepositionCosmicsModule module
         * @param reader PrimariesReader instance to fetch particle lists from
         */
        explicit PrimariesGeneratorAction(const Configuration& config, std::shared_ptr<PrimariesReader> reader);

        /**
         * @brief Generate the particle for every event
         */
        void GeneratePrimaries(G4Event*) override;

    private:
        /**
         * @brief helper method to check if particle is to be dispateched within the defined world volume
         * @param  pos Initial position of the primary particle
         * @return     True if within the world volume, false otherwise
         */
        static bool check_vertex_inside_world(const G4ThreeVector& pos);

        std::unique_ptr<G4ParticleGun> particle_gun_;
        std::shared_ptr<PrimariesReader> reader_;
    };
} // namespace allpix

#endif /* ALLPIX_PRIMARIES_DEPOSITION_MODULE_GENERATOR_ACTION_H */
