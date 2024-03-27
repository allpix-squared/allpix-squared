/**
 * @file
 * @brief Defines a user hook for Geant4 stepping action to catch problematic events and abort them
 *
 * @copyright Copyright (c) 2023-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef StepInfoUserHookG4_H
#define StepInfoUserHookG4_H 1

#include "G4Step.hh"
#include "G4UserSteppingAction.hh"

namespace allpix {
    /**
     * @brief Allows access to the info of each Geant4 step
     */
    class StepInfoUserHookG4 : public G4UserSteppingAction {
    public:
        /**
         * @brief Default constructor
         */
        explicit StepInfoUserHookG4() = default;

        /**
         * @brief Default destructor
         */
        ~StepInfoUserHookG4() override = default;

        /**
         * @brief Called for every step in Geant4
         * @param aStep The pointer to the G4Step for which this routine is called
         */
        void UserSteppingAction(const G4Step* aStep) override;
    };

} // namespace allpix
#endif /* StepInfoUserHookG4_H */
