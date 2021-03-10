/**
 * @file
 * @brief Implementation of MTRunManager
 * @copyright Copyright (c) 2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "MTRunManager.hpp"
#include "WorkerRunManager.hpp"

using namespace allpix;

namespace {
    G4Mutex worker_seed_mutex = G4MUTEX_INITIALIZER;
}

G4ThreadLocal WorkerRunManager* MTRunManager::worker_run_manager_ = nullptr;

void MTRunManager::Run(G4int n_event, uint64_t seed1, uint64_t seed2) { // NOLINT
    {
        G4AutoLock l(&worker_seed_mutex);

        worker_run_manager_->seedsQueue.push(static_cast<long>(seed1));
        worker_run_manager_->seedsQueue.push(static_cast<long>(seed2));

        // for book keeping
        numberOfEventProcessed += n_event;
    }

    // redirect the call to the correct manager responsible for this thread
    worker_run_manager_->BeamOn(n_event);
}

void MTRunManager::Initialize() {
    G4MTRunManager::Initialize();

    G4bool cond = ConfirmBeamOnCondition();
    if(cond) {
        G4MTRunManager::ConstructScoringWorlds();
        G4MTRunManager::RunInitialization();

        // This is needed to draw random seeds and fill the internal seed array
        // use nSeedsMax to fill as much as possible now and hopefully avoid
        // refilling later
        G4MTRunManager::DoEventLoop(nSeedsMax, nullptr, 0);

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
