/**
 * @file
 * @brief Definition of [DepositionPrimaries] module
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_GENERATOR_DEPOSITION_MODULE_H
#define ALLPIX_GENERATOR_DEPOSITION_MODULE_H

#include "../DepositionGeant4/DepositionGeant4Module.hpp"
#include "PrimariesReader.hpp"

#include <mutex>

namespace allpix {
    /**
     * @ingroup SequentialModules
     * @brief SequentialModule to do function
     *
     * More detailed explanation of module
     */
    class DepositionGeneratorModule : public DepositionGeant4Module {
        friend class CosmicsGeneratorActionG4;

    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         */
        DepositionGeneratorModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager);

        /**
         * @brief Initializes the physics list of processes and constructs the particle source
         */
        void initialize() override;

    private:
        void initialize_g4_action() override;
        std::shared_ptr<PrimariesReader> reader_;
    };

} // namespace allpix
#endif /* ALLPIX_GENERATOR_DEPOSITION_MODULE_H */
