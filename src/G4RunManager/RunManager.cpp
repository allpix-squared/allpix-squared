/**
 * @file
 * @brief Implementation of RunManager
 * @copyright Copyright (c) 2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "RunManager.hpp"
#include "WorkerRunManager.hpp"

using namespace allpix;

G4ThreadLocal WorkerRunManager* RunManager::worker_run_manager_ = nullptr;

void RunManager::BeamOn(G4int n_event, const char* macroFile, G4int n_select) {
    if (!worker_run_manager_) {
        // construct a new thread worker
        worker_run_manager_ = WorkerRunManager::GetNewInstanceForThread();
    }

    // Draw the nessecary seeds so that each event will be seeded
    // TODO: maybe we only need to seed the RNG once for a worker run and not for each loop iteration
    G4RNGHelper* helper = G4RNGHelper::GetInstance();
    for (G4int i = 0; i < n_event; ++i) {
        G4int idx_rndm = nSeedsPerEvent*nSeedsUsed;
        long s1 = helper->GetSeed(idx_rndm), s2 = helper->GetSeed(idx_rndm+1);
        worker_run_manager_->seedsQueue.push(s1);
        worker_run_manager_->seedsQueue.push(s2);

        nSeedsUsed++;

        if(nSeedsUsed==nSeedsFilled) {
            // The RefillSeeds call will refill the array with 1024 new entries
            // the number of seeds refilled = numberOfEventToBeProcessed - nSeedsFilled
            numberOfEventToBeProcessed = nSeedsFilled + 1024;
            RefillSeeds();
        }
    }

    // for book keeping
    numberOfEventProcessed += n_event;

    // redirect the call to the correct manager responsible for this thread
    worker_run_manager_->BeamOn(n_event, macroFile, n_select);
}

void RunManager::Initialize() {
    G4MTRunManager::Initialize();

    // This is needed to draw random seeds and fill the internal seed array
    // use nSeedsMax to fill as much as possible now and hopefully avoid
    // refilling later
    G4MTRunManager::InitializeEventLoop(nSeedsMax, nullptr, 0);
}

void RunManager::TerminateForThread() {
    // thread local instance
    worker_run_manager_->RunTermination();
    delete worker_run_manager_;
    worker_run_manager_ = nullptr;
}