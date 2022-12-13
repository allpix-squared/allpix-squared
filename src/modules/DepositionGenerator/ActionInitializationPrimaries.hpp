/**
 * @file
 * @brief Defines the worker initialization class
 *
 * @copyright Copyright (c) 2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_PRIMARIES_DEPOSITION_MODULE_ACTION_INITIALIZATION_H
#define ALLPIX_PRIMARIES_DEPOSITION_MODULE_ACTION_INITIALIZATION_H

#include <G4VUserActionInitialization.hh>

#include "core/config/Configuration.hpp"

#include "../DepositionGeant4/SetTrackInfoUserHookG4.hpp"

namespace allpix {
    /**
     * @brief Initializer for the generator actions, required for \ref RunManager
     */
    template <class GEN> class ActionInitializationPrimaries : public G4VUserActionInitialization {
    public:
        explicit ActionInitializationPrimaries(const Configuration& config, std::shared_ptr<PrimariesReader> reader)
            : config_(config), reader_(std::move(reader)){};

        /**
         * @brief Build the user action to be executed by the worker
         * @note All SetUserAction must be called from here
         */
        void Build() const override {
            // primary particles generator
            SetUserAction(new GEN(config_, reader_));

            // tracker hook
            SetUserAction(new SetTrackInfoUserHookG4());
        };

    private:
        const Configuration& config_;
        std::shared_ptr<PrimariesReader> reader_;
    };
} // namespace allpix

#endif /* ALLPIX_PRIMARIES_DEPOSITION_MODULE_ACTION_INITIALIZATION_H */
