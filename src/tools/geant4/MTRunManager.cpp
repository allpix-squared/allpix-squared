/**
 * @file
 * @brief Implementation of MTRunManager
 *
 * @copyright Copyright (c) 2019-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "MTRunManager.hpp"
#include "G4LoggingDestination.hpp"
#include "WorkerRunManager.hpp"

#include <magic_enum/magic_enum.hpp>

#include <G4StateManager.hh>
#include <G4UImanager.hh>

#include "tools/geant4/G4ExceptionHandler.hpp"

using namespace allpix;

G4ThreadLocal WorkerRunManager* MTRunManager::worker_run_manager_ = nullptr;

MTRunManager::MTRunManager() {
    G4UImanager* ui_g4 = G4UImanager::GetUIpointer();
    ui_g4->SetCoutDestination(G4LoggingDestination::getInstance());
    // Set exception handler for Geant4 exceptions:
    G4StateManager::GetStateManager()->SetExceptionHandler(new G4ExceptionHandler());
}

void MTRunManager::Run(G4int n_event, uint64_t seed1, uint64_t seed2) { // NOLINT

    LOG(DEBUG) << "Current Geant4 state: " << magic_enum::enum_name(G4StateManager::GetStateManager()->GetCurrentState());

    // Seed the worker run manager for this event:
    worker_run_manager_->seedsQueue.push(static_cast<long>(seed1 % LONG_MAX));
    worker_run_manager_->seedsQueue.push(static_cast<long>(seed2 % LONG_MAX));

    // redirect the call to the correct manager responsible for this thread
    worker_run_manager_->BeamOn(n_event);
}

void MTRunManager::Initialize() {
    G4MTRunManager::Initialize();

    G4bool cond = ConfirmBeamOnCondition();
    if(cond) {
        G4MTRunManager::ConstructScoringWorlds();
        G4MTRunManager::RunInitialization();

        // Prepare UI commands for workers
        PrepareCommandsStack();
    }
}

void MTRunManager::InitializeForThread() { // NOLINT
    if(worker_run_manager_ == nullptr) {
        // construct a new thread worker
        worker_run_manager_ = WorkerRunManager::GetNewInstanceForThread();
    }
}

void MTRunManager::TerminateForThread() { // NOLINT
    // thread local instance
    if(worker_run_manager_ != nullptr) {
        worker_run_manager_->RunTermination();
        delete worker_run_manager_;
        worker_run_manager_ = nullptr;
    }
}

void MTRunManager::AbortRun(bool softAbort) {
    // Call the AbortRun() method of the worker
    worker_run_manager_->AbortRun(softAbort);
}
