/**
 * @file
 * @brief Defines the particle generator
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_SIMPLE_DEPOSITION_MODULE_GENERATOR_ACTION_H
#define ALLPIX_SIMPLE_DEPOSITION_MODULE_GENERATOR_ACTION_H

#include <memory>

#include <G4GeneralParticleSource.hh>
#include <G4ParticleDefinition.hh>
#include <G4SDManager.hh>
#include <G4ThreeVector.hh>
#include <G4TwoVector.hh>
#include <G4VUserPrimaryGeneratorAction.hh>

#include "core/config/Configuration.hpp"

namespace allpix {
    /**
     * @brief Generates the particles in every event
     */
    class GeneratorActionG4 : public G4VUserPrimaryGeneratorAction {
    public:
        /**
         * @brief Different types of particle sources
         */
        enum class SourceType {
            MACRO,  ///< Source defined by a macro file
            BEAM,   ///< Beam particle source
            SPHERE, ///< Spherical particle source
            SQUARE, ///< Square particle source
            POINT,  ///< Point source
        };

        /**
         * @brief Different shapes of particle beams
         */
        enum class BeamShape {
            CIRCLE,    ///< Circular beam
            ELLIPSE,   ///< Elliptical beam
            RECTANGLE, ///< Rectangular beam
        };

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

        static std::map<std::string, std::tuple<int, int, int, double>> isotopes_;

        const Configuration& config_;

        std::string particle_type_;

        bool initialize_ion_as_particle_{false};

        double time_{0.0};
        double time_window_{0.0};
    };

    /**
     * @brief Creates and initialize the GPS messenger on master before workers use it
     */
    class GeneratorActionInitializationMaster {
    public:
        /**
         * Constructs a particle source for master and apply common UI macros if requested.
         */
        explicit GeneratorActionInitializationMaster(const Configuration& config);

    private:
        std::unique_ptr<G4GeneralParticleSource> particle_source_;
    };
} // namespace allpix

#endif /* ALLPIX_SIMPLE_DEPOSITION_MODULE_GENERATOR_ACTION_H */
