/**
 * @file
 * @brief The WorkerRunManager class, run manager for Geant4 that works on separate thread.
 *
 * @copyright Copyright (c) 2019-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_WORKER_RUN_MANAGER_H
#define ALLPIX_WORKER_RUN_MANAGER_H

#include <G4Version.hh>
#include <G4WorkerRunManager.hh>

namespace allpix {

    class MTRunManager;

    /**
     * @brief Run manager for Geant4 that can be used by multiple threads where each thread will have its own instance.
     *
     * This manager overrides G4WorkerRunManager behaviour so it can be used on user defined threads. Therefore, there
     * is no dependency on the master run manager except only in initialization.
     * APIs inherited from G4WorkerRunManager which communicate with master run manager are suppressed because they
     * are not needed anymore. This manager assumes that the client is only interested in its own results and it is
     * independent from other instances running on different threads.
     */
    class WorkerRunManager : public G4WorkerRunManager {
        friend class MTRunManager;

    public:
        ~WorkerRunManager() override;

        /**
         * @brief Executes specified number of events.
         *
         * Executes the specified number of events. Reimplemented to execute UI commands and maybe
         * reinitialize workspace if there are changes between multiple calls.
         */
        void BeamOn(G4int n_event, const char* macroFile = nullptr, G4int n_select = -1) override; // NOLINT

        void InitializeGeometry() override;

        /**
         * @brief Overriding G4RunManager::AbortRun so as to reset the state to G4State_Idle in order to allow the next event
         * to run BeamOn
         */
        void AbortRun(G4bool softAbort) override; // NOLINT

        /**
         * @brief Factory method to create new worker for calling thread.
         *
         * Creates a new worker and initialize it to be used by the calling thread.
         */
        static WorkerRunManager* GetNewInstanceForThread(); // NOLINT

    protected:
        WorkerRunManager();

        /**
         * @brief Run the event loop for specified number of events.
         * @param n_event number of events
         * @param macroFile Possible pointer to a macro file, passed on to Geant4
         * @param n_select Optional n_select parameter passed on to Geant4
         *
         * Run the event loop. Everything is the same as base implementation except that we keep
         * the seedsQueue since the master manager has already pushed the seeds in it.
         */
        void DoEventLoop(G4int n_event, const char* macroFile = nullptr, G4int n_select = -1) override; // NOLINT

        /**
         * @brief Previously used to communicate work with master manager.
         *
         * Thread loop for receiving work from master run manager. It will now do nothing.
         */
        void DoWork() override {}

        /**
         * @brief Constructs an event object and set the seeds for RNG.
         * @param i_event the event number.
         *
         * Creates a new G4Event object and set its event number, seeds for the thread RNG.
         */
        G4Event* GenerateEvent(G4int i_event) override;

#if G4VERSION_NUMBER < 1130
        /**
         * @brief Previously used to merge the partial results obtained by this manager and the master.
         *
         * Merge the run results with the master results. It will now do nothing.
         */
        void MergePartialResults() override {}
#else
        /**
         * @brief Previously used to merge the partial results obtained by this manager and the master.
         *
         * Merge the run results with the master results. It will now do nothing.
         */
        void MergePartialResults(G4bool) override {}
#endif
    };
} // namespace allpix

#endif /* ALLPIX_WORKER_RUN_MANAGER_H */
