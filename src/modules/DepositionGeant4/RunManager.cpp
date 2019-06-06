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

void RunManager::Initialize()
{
    G4MTRunManager::Initialize();

    // This is needed to draw random seeds and fill the internal seed array
    // use nSeedsMax to fill as much as possible now and hopefully avoid
    // refilling later
    G4MTRunManager::InitializeEventLoop(nSeedsMax, nullptr, 0);
}

void RunManager::TerminateForThread()
{
    worker_run_manager_->RunTermination();
    delete worker_run_manager_;
    worker_run_manager_ = nullptr;
}