/**
 * @file
 * @brief The RunManager class, defines a custom Geant4 sequential RunManager that is compatible with MTRunManager.
 *
 * @copyright Copyright (c) 2019-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_RUN_MANAGER_H
#define ALLPIX_RUN_MANAGER_H

#include <G4RunManager.hh>

namespace allpix {
    /**
     * @brief A wrapper around G4RunManager that allows us to use our own event seeds
     */
    class RunManager : public G4RunManager {
    public:
        RunManager();
        ~RunManager() override = default;
        RunManager(const RunManager&) = default;
        RunManager& operator=(const RunManager&) = default;
        RunManager(RunManager&&) = default;
        RunManager& operator=(RunManager&&) = default;

        /**
         * @brief Wrapper around G4RunManager::BeamOn that seeds the RNG before calling BeamOn
         * @param n_event number of events (particles) to simulate in one call to BeamOn.
         * @param seed1 First event seed
         * @param seed2 Second event seed
         */
        void Run(G4int n_event, uint64_t seed1, uint64_t seed2); // NOLINT

        /**
         * @brief Overriding G4RunManager::AbortRun so as to reset the state to G4State_Idle in order to allow the next event
         * to run BeamOn
         */
        void AbortRun(G4bool softAbort) override;
    };
} // namespace allpix

#endif /* ALLPIX_RUN_MANAGER_H */
