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

// NOLINTNEXTLINE(readability-identifier-naming)
void MTRunManager::Run(G4int allpix_event, G4int n_event) {
    {
        G4AutoLock l(&worker_seed_mutex);
        // Draw the nessecary seeds so that each event will be seeded
        G4RNGHelper* helper = G4RNGHelper::GetInstance();
        G4int idx_rndm = nSeedsPerEvent * (allpix_event - 1);
        long s1 = helper->GetSeed(idx_rndm), s2 = helper->GetSeed(idx_rndm + 1);
        worker_run_manager_->seedsQueue.push(s1);
        worker_run_manager_->seedsQueue.push(s2);

        nSeedsUsed++;

        if(nSeedsUsed == nSeedsFilled) {
            // The RefillSeeds call will refill the array with 1024 new entries
            // the number of seeds refilled = numberOfEventToBeProcessed - nSeedsFilled
            numberOfEventToBeProcessed = nSeedsFilled + nSeedsMax;
            RefillSeeds();
        }

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
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
void MTRunManager::InitializeForThread() {
    if(worker_run_manager_ == nullptr) {
        // construct a new thread worker
        worker_run_manager_ = WorkerRunManager::GetNewInstanceForThread();
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
void MTRunManager::TerminateForThread() {
    // thread local instance
    if(worker_run_manager_ != nullptr) {
        worker_run_manager_->RunTermination();
        delete worker_run_manager_;
        worker_run_manager_ = nullptr;
    }
}