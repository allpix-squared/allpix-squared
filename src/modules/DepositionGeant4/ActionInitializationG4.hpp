/**
 * @file
 * @brief Defines the worker initialization class
 *
 * @copyright Copyright (c) 2019-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_SIMPLE_DEPOSITION_MODULE_ACTION_INITIALIZATION_H
#define ALLPIX_SIMPLE_DEPOSITION_MODULE_ACTION_INITIALIZATION_H

#include <G4VUserActionInitialization.hh>

#include "core/config/Configuration.hpp"

#include "SetTrackInfoUserHookG4.hpp"
#include "StepInfoUserHookG4.hpp"

namespace allpix {
    /**
     * @brief Initializer for the tracker and generator actions, required for \ref RunManager
     */
    template <class GEN, class INIT> class ActionInitializationG4 : public G4VUserActionInitialization {
    public:
        explicit ActionInitializationG4(const Configuration& config) : config_(config) {};

        /**
         * @brief Build the user action to be executed by the worker
         * @note All SetUserAction must be called from here
         */
        void Build() const override {
            // primary particles generator
            SetUserAction(new GEN(config_));

            // tracker hook
            SetUserAction(new SetTrackInfoUserHookG4());

            // step hook
            SetUserAction(new StepInfoUserHookG4());
        };

        /**
         * @brief Constructs a actions for Master
         *
         * Used to set up a particle source for master when UI commands are used
         */
        void BuildForMaster() const override {
            // UI Commands are applied through the GPS messenger which is a singleton instance
            // that modifies shared resources across threads and therefore must only be executed
            // on the master thread.
            // We force the creation of the messenger early on master and apply UI commands so
            // that when workers are ready to use their own instances of GPS class they are
            // initialized with common UI commands
            static INIT generator(config_);
            (void)generator;
        }

    private:
        const Configuration& config_;
    };
} // namespace allpix

#endif /* ALLPIX_SIMPLE_DEPOSITION_MODULE_ACTION_INITIALIZATION_H */
