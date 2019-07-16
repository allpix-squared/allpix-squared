/**
 * @file
 * @brief The RunManager class, defines a custom Geant4 sequential RunManager that is compatiable with MTRunManager.
 * @copyright Copyright (c) 2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_RUN_MANAGER_H
#define ALLPIX_RUN_MANAGER_H

#include <array>

#include <G4RNGHelper.hh>
#include <G4RunManager.hh>

namespace allpix {
    /**
     * @brief A modfied version of G4RunManager that is totally compatiable with G4MTRunManager.
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
        virtual ~RunManager() = default;

        /**
         * @brief Wrapper around G4RunManager BeamOn that seeds the RNG before actually calling
         * BeamOn
         */
        virtual void BeamOn(G4int n_event, const char *macro_file, G4int n_select) override;

    private:
        static constexpr G4int number_seeds_per_event_{2};
        CLHEP::HepRandomEngine* master_random_engine_{nullptr};
        CLHEP::HepRandomEngine* event_random_engine_{nullptr};
        std::array<double, number_seeds_per_event_> seed_array_;
    };
} // namespace allpix

#endif /* ALLPIX_RUN_MANAGER_H */
