/**
 * @file
 * @brief Implementation of RunManager
 *
 * @copyright Copyright (c) 2019-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "RunManager.hpp"

#include <array>
#include <climits>

#include <G4ApplicationState.hh>
#include <G4RunManager.hh>
#include <G4StateManager.hh>
#include <G4UImanager.hh>
#include <G4ios.hh>
#include <Randomize.hh>
#include <magic_enum/magic_enum.hpp>

#include "core/utils/log.h"
#include "tools/geant4/G4ExceptionHandler.hpp"
#include "tools/geant4/G4LoggingDestination.hpp"

using namespace allpix;

RunManager::RunManager() {
    G4UImanager* ui_g4 = G4UImanager::GetUIpointer();
    ui_g4->SetCoutDestination(G4LoggingDestination::getInstance());
    // Set exception handler for Geant4 exceptions:
    G4StateManager::GetStateManager()->SetExceptionHandler(new G4ExceptionHandler());
}

void RunManager::Run(G4int n_event, uint64_t seed1, uint64_t seed2) { // NOLINT

    LOG(DEBUG) << "Current Geant4 state: " << magic_enum::enum_name(G4StateManager::GetStateManager()->GetCurrentState());

    // Set the event seeds - with a zero-terminated list:
    std::array<long, 3> seeds{static_cast<long>(seed1 % LONG_MAX), static_cast<long>(seed2 % LONG_MAX), 0};
    G4Random::setTheSeeds(seeds.data(), -1);

    // Call the RunManager's BeamOn
    G4RunManager::BeamOn(n_event);
}

void RunManager::AbortRun(bool softAbort) {
    // This method is valid only for GeomClosed or EventProc state
    G4ApplicationState const currentState = G4StateManager::GetStateManager()->GetCurrentState();
    if(currentState == G4State_GeomClosed || currentState == G4State_EventProc) {
        runAborted = true;
        if(currentState == G4State_EventProc && !softAbort) {
            currentEvent->SetEventAborted();
            eventManager->AbortCurrentEvent();
            LOG(TRACE) << "Aborted Geant4 event";
        }
        // Ready for new event, set the state back to G4State_Idle
        G4StateManager::GetStateManager()->SetNewState(G4State_Idle);
        LOG(DEBUG) << "Reset Geant4 state to "
                   << magic_enum::enum_name(G4StateManager::GetStateManager()->GetCurrentState());
    } else {
        LOG(WARNING) << "Run is not in progress. AbortRun() ignored." << G4endl;
    }
}
