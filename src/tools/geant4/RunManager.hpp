/**
 * @file
 * @brief The RunManager class, defines a custom Geant4 sequential RunManager that is compatible with MTRunManager.
 * @copyright Copyright (c) 2019-2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_RUN_MANAGER_H
#define ALLPIX_RUN_MANAGER_H

#include <G4RunManager.hh>

namespace allpix {
    /**
     * @brief A modified version of G4RunManager that is totally compatible with G4MTRunManager.
     *
     * This manager uses the same event seeding mechanism as the G4MTRunManager to ensure that
     * they can be used interchangeably producing the same results.
     * It uses two random number generators; one used by Geant4 library while the other one is
     * only used to generate seeds for the first one. This way for each event the generator is
     * seeded with a unique seed drawn from the other generator.
     */
    class RunManager : public G4RunManager {
    public:
        RunManager() = default;
        ~RunManager() override = default;

        /**
         * @brief Wrapper around \ref G4RunManager BeamOn that seeds the RNG before calling BeamOn
         * @param n_event number of events to simulate in one run.
         * @param seed1 First seed for worker run manager for this event
         * @param seed2 Second seed for worker run manager for this event
         */
        void Run(G4int n_event, uint64_t seed1, uint64_t seed2); // NOLINT

    private:
        static constexpr G4int number_seeds_per_event_{2};
    };
} // namespace allpix

#endif /* ALLPIX_RUN_MANAGER_H */
