/**
 * @file
 * @brief Implementation of RunManager
 *
 * @copyright Copyright (c) 2019-2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "RunManager.hpp"
#include "G4LoggingDestination.hpp"

#include <array>

#include <G4StateManager.hh>
#include <G4UImanager.hh>

#include "tools/geant4/G4ExceptionHandler.hpp"

using namespace allpix;

RunManager::RunManager() {
    G4UImanager* ui_g4 = G4UImanager::GetUIpointer();
    ui_g4->SetCoutDestination(G4LoggingDestination::getInstance());
    // Set exception handler for Geant4 exceptions:
    G4StateManager::GetStateManager()->SetExceptionHandler(new G4ExceptionHandler());
}

void RunManager::Run(G4int n_event, uint64_t seed1, uint64_t seed2) { // NOLINT

    auto printMeNicely = []() {
        G4ApplicationState currentState = G4StateManager::GetStateManager()->GetCurrentState();

        std::string stateText = "";
        if(currentState == G4State_PreInit) {
            stateText = "G4State_PreInit";
        } else if(currentState == G4State_Init) {
            stateText = "G4State_Init";
        } else if(currentState == G4State_Idle) {
            stateText = "G4State_Idle";
        } else if(currentState == G4State_GeomClosed) {
            stateText = "G4State_GeomClosed";
        } else if(currentState == G4State_EventProc) {
            stateText = "G4State_EventProc";
        } else if(currentState == G4State_Quit) {
            stateText = "G4State_Quit";
        } else if(currentState == G4State_Abort) {
            stateText = "G4State_Abort";
        } else {
            stateText = "Unknown";
        }
        return stateText;
    };

    LOG(WARNING) << "Current state: " << printMeNicely();
    // Set the event seeds - with a zero-terminated list:
    std::array<long, 3> seeds{static_cast<long>(seed1 % LONG_MAX), static_cast<long>(seed2 % LONG_MAX), 0};
    G4Random::setTheSeeds(&seeds[0], -1);

    // Call the RunManager's BeamOn
    G4RunManager::BeamOn(n_event);
}

void RunManager::AbortRun(bool softAbort) {
    // This method is valid only for GeomClosed or EventProc state
    G4ApplicationState currentState = G4StateManager::GetStateManager()->GetCurrentState();
    if(currentState == G4State_GeomClosed || currentState == G4State_EventProc) {
        runAborted = true;
        if(currentState == G4State_EventProc && !softAbort) {
            currentEvent->SetEventAborted();
            eventManager->AbortCurrentEvent();
        }
        // Ready for new event, set the state back to G4State_Idle
        G4StateManager::GetStateManager()->SetNewState(G4State_Idle);
    } else {
        LOG(WARNING) << "Run is not in progress. AbortRun() ignored." << G4endl;
    }
}
