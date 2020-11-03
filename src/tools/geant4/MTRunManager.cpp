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

void MTRunManager::FillWorkerSeedsMap(G4int n_event) { // NOLINT
    // Fill the auxiliary array with new random numbers drawn from the main engine
    auto* engine = G4Random::getTheEngine();
    engine->flatArray(nSeedsPerEvent * n_event, randDbl);

    // Fill the mapping between event number and pair of seeds to be used
    for(G4int i = 0, event_number = nSeedsFilled + 1; i < nSeedsPerEvent * n_event; i += 2, event_number++) {
        // Convert floating points to long
        auto pairs =
            std::make_pair(static_cast<long>(100000000L * randDbl[i]), static_cast<long>(100000000L * randDbl[i + 1]));
        workers_seeds_.insert(std::make_pair(event_number, pairs));
    }

    // Update the number of filled seeds so far
    nSeedsFilled += n_event;
}

void MTRunManager::Run(G4int allpix_event, G4int n_event) { // NOLINT
    {
        G4AutoLock l(&worker_seed_mutex);

        // Draw the necessary seeds so that each event will be seeded
        auto itr = workers_seeds_.find(allpix_event);
        if(itr == workers_seeds_.end()) {
            // We need to fill more seeds
            FillWorkerSeedsMap(nSeedsMax);
            itr = workers_seeds_.find(allpix_event);
        }

        auto seeds = itr->second;
        worker_run_manager_->seedsQueue.push(seeds.first);
        worker_run_manager_->seedsQueue.push(seeds.second);
        workers_seeds_.erase(itr);

        nSeedsUsed++;

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

G4bool MTRunManager::InitializeSeeds(G4int nevts) { // NOLINT
    FillWorkerSeedsMap(nevts);
    return true;
}
