/**
 * @file
 * @brief The WorkerRunManager class, run manager for Geant4 that works on seperate thread.
 * @copyright Copyright (c) 2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_WORKER_RUN_MANAGER_H
#define ALLPIX_WORKER_RUN_MANAGER_H

#include <G4WorkerRunManager.hh>

namespace allpix {

    class MTRunManager;

    /**
     * @brief Run manager for Geant4 that can be used by multiple threads where each thread will have its own instance.
     *
     * This manager overrides \ref G4WorkerRunManager behaviour so it can be used on user defined threads. Therefore, there
     * is no dependency on the master run manager except only in initialization.
     * APIs inherited from \ref G4WorkerRunManager which communicate with master run manager are suppressed because they
     * are not needed anymore. This manager assumes that the client is only interested into its own results and it is
     * independent from other instances running on different threads.
     */
    class WorkerRunManager : public G4WorkerRunManager {
        friend class MTRunManager;

    public:
        virtual ~WorkerRunManager();

        /**
         * @brief Executes specified number of events.
         *
         * Executes the specified number of events. Reimplemented to execute UI commands and maybe
         * reinitialize workspace if there are changes between multiple calls.
         */
        virtual void BeamOn(G4int n_event, const char* macroFile = nullptr, G4int n_select = -1) override;

        void InitializeGeometry() override;

        /**
         * @brief Factory method to create new worker for calling thread.
         *
         * Creates a new worker and initialize it to be used by the calling thread.
         */
        static WorkerRunManager* GetNewInstanceForThread();

    protected:
        WorkerRunManager() = default;

        /**
         * @brief Run the event loop for specified number of events.
         * @param n_event number of events
         *
         * Run the event loop. Everything is the same as base implementation except that we keep
         * the seedsQueue since the master manager has already pushed the seeds in it.
         */
        virtual void DoEventLoop(G4int n_event, const char* macroFile = nullptr, G4int n_select = -1) override;

        /**
         * @brief Previously used to communicate work with master manager.
         *
         * Thread loop for receiving work from master run manager. It will now do nothing.
         */
        virtual void DoWork() override {}

        /**
         * @brief Constructs an event object and set the seeds for RNG.
         * @param i_event the event number.
         *
         * Creats a new \ref G4Event object and set its event number, seeds for the thread RNG.
         */
        virtual G4Event* GenerateEvent(G4int i_event) override;

        /**
         * @brief Previously used to merge the partial results obtained by this manager and the master.
         *
         * Merge the run results with the master results. It will now do nothing.
         */
        virtual void MergePartialResults() override {}
    };
} // namespace allpix

#endif /* ALLPIX_WORKER_RUN_MANAGER_H */
