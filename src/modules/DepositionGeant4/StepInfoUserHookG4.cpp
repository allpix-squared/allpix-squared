/**
 * @file
 * @brief Implements a user hook for Geant4 stepping action to catch problematic events and abort them
 *
 * @copyright Copyright (c) 2023-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <G4Step.hh>

#include "DepositionGeant4Module.hpp"
#include "StepInfoUserHookG4.hpp"

#include "core/utils/log.h"
#include "tools/geant4/RunManager.hpp"

using namespace allpix;

void StepInfoUserHookG4::UserSteppingAction(const G4Step* aStep) {
    if(aStep->GetStepLength() < 0) {
        LOG(WARNING) << "Negative step length found; aborting event.";

        // Load the G4 run manager (which is owned by the geometry builder), and call AbortRun() to abort the current event
        G4RunManager::GetRunManager()->AbortRun();
    }
}
