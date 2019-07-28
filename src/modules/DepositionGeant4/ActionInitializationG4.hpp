/**
 * @file
 * @brief Defines the worker initialization object
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_SIMPLE_DEPOSITION_MODULE_ACTION_INITIALIZATION_H
#define ALLPIX_SIMPLE_DEPOSITION_MODULE_ACTION_INITIALIZATION_H

#include <G4VUserActionInitialization.hh>

#include "core/config/Configuration.hpp"

#include "GeneratorActionG4.hpp"
#include "SetTrackInfoUserHookG4.hpp"

namespace allpix {
    class DepositionGeant4Module;
    /**
     * @brief Initializer for the tracker and generator actions, required for \ref RunManager
     */
    class ActionInitializationG4 : public G4VUserActionInitialization {
    public:
        explicit ActionInitializationG4(const Configuration& config, DepositionGeant4Module* module)
            : config_(config), module_(module){};

        /**
         * @brief Build the user action to be executed by the worker
         * @note All SetUserAction must be called from here
         */
        void Build() const override {
            // primary particles generator
            SetUserAction(new GeneratorActionG4(config_));

            // tracker hook
            SetUserAction(new SetTrackInfoUserHookG4(module_));
        };

    private:
        const Configuration& config_;

        // Raw ptr to track info manager to create instances of TrackInfoG4
        DepositionGeant4Module* module_;
    };
} // namespace allpix

#endif /* ALLPIX_SIMPLE_DEPOSITION_MODULE_ACTION_INITIALIZATION_H */
