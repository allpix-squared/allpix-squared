/**
 * @file
 * @brief Defines the worker initialization class
 *
 * @copyright Copyright (c) 2022-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_PRIMARIES_DEPOSITION_MODULE_ACTION_INITIALIZATION_H
#define ALLPIX_PRIMARIES_DEPOSITION_MODULE_ACTION_INITIALIZATION_H

#include <G4VUserActionInitialization.hh>

#include "core/config/Configuration.hpp"

#include "modules/DepositionGeant4/SetTrackInfoUserHookG4.hpp"
#include "modules/DepositionGeant4/StepInfoUserHookG4.hpp"

namespace allpix {
    class PrimariesReader;

    /**
     * @brief Initializer for the generator actions, required for \ref RunManager
     *
     * We directly inherit from Geant4's action initialization class instead of the DepositionGeant4 class since the latter
     * provides functionality specific to that module which we do not need - while we need to be able to pass an additional
     * parameter, the PrimariesReader, to the action constructor.
     */
    template <class GEN> class ActionInitializationPrimaries : public G4VUserActionInitialization {
    public:
        explicit ActionInitializationPrimaries(const Configuration& config, std::shared_ptr<PrimariesReader>& reader)
            : config_(config), reader_(reader) {};

        /**
         * @brief Build the user action to be executed by the worker
         * @note All SetUserAction must be called from here
         */
        void Build() const override {
            // primary particles generator
            SetUserAction(new GEN(config_, reader_));

            // tracker hook
            SetUserAction(new SetTrackInfoUserHookG4());

            // step hook
            SetUserAction(new StepInfoUserHookG4());
        };

    private:
        const Configuration& config_;
        std::shared_ptr<PrimariesReader> reader_;
    };
} // namespace allpix

#endif /* ALLPIX_PRIMARIES_DEPOSITION_MODULE_ACTION_INITIALIZATION_H */
