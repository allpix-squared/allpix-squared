/**
 * @file
 * @brief Implements a user hook for Geant4 to assign custom track information via TrackInfoG4 objects
 *
 * @copyright Copyright (c) 2018-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "StepInfoUserHookG4.hpp"
#include "DepositionGeant4Module.hpp"

#include "tools/geant4/RunManager.hpp"

using namespace allpix;

void StepInfoUserHookG4::UserSteppingAction(const G4Step* aStep) {
    LOG(DEBUG) << "Step length: " << aStep->GetStepLength();
    if(aStep->GetStepLength() < 0) {
        LOG(WARNING) << "Negative step length found; aborting event.";

        // Load the G4 run manager (which is owned by the geometry builder), and call AbortEvent()
        G4RunManager::GetRunManager()->AbortEvent();
    }
}
