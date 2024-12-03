/**
 * @file
 * @brief Definition of the DepositionGenerator module
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_GENERATOR_DEPOSITION_MODULE_H
#define ALLPIX_GENERATOR_DEPOSITION_MODULE_H

#include "PrimariesReader.hpp"
#include "modules/DepositionGeant4/DepositionGeant4Module.hpp"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to read primary particles from MC generators
     *
     * This module reads primary particles produced by Monte Carlo generators from a data file and dispatches them using a
     * Geant4 ParticleGun. The \ref DepositionGeant4Module then performs the tracking of the particles through the setup and
     * deposits electron/hole pairs.
     */
    class DepositionGeneratorModule : public DepositionGeant4Module {
    public:
        /**
         * @brief Constructor for the DepositionGenerator module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         */
        DepositionGeneratorModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager);

        /**
         * @brief Initializes the file reader according to the configuration and then calls \ref
         * DepositionGeant4Module::initialize() to initialize physics lists etc.
         */
        void initialize() override;

        /**
         * @brief Passes the currently processed event number to the primary particle reader module and calls \ref
         * DepositionGeant4Module::run.
         * @param event  Pointer to the currently processed event
         */
        void run(Event* event) override;

    private:
        /**
         * @brief Helper method to initialize the generator action for dispatching particles via a particle gun
         */
        void initialize_g4_action() override;

        // The file reader for primary particles
        std::shared_ptr<PrimariesReader> reader_;
        PrimariesReader::FileModel file_model_;
    };

} // namespace allpix
#endif /* ALLPIX_GENERATOR_DEPOSITION_MODULE_H */
