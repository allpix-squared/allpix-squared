/**
 * @file
 * @brief The MTRunManager class, defines a custom Geant4 RunManager that works with Allpix threads.
 * @copyright Copyright (c) 2019-2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MT_RUN_MANAGER_H
#define ALLPIX_MT_RUN_MANAGER_H

#include <unordered_map>

#include <G4MTRunManager.hh>

#include "SensitiveDetectorAndFieldConstruction.hpp"

namespace allpix {

    class WorkerRunManager;

    /**
     * @brief A custom run manager for Geant4 that can work with external threads and be used concurrently.
     *
     * This manager overrides G4MTRunManager class so it doesn't create its own thread and work with the threads
     * already created by \ref ModuleManager class. Also, it provides a concurrent API that can be used by
     * multiple threads safely at the same time.
     * Most of the APIs defined by G4MTRunManager are overridden to simply do nothing since this custom run
     * manager doesn't operate its own event loop and assumes it is part of the client event loop and the results
     * of each event are independent from each other. Also, this  manager doesn't maintain any threads, it only
     * maintains the worker managers which are allocated on a per thread basis.
     */
    class MTRunManager : public G4MTRunManager {
        friend class WorkerRunManager;

    public:
        MTRunManager();
        ~MTRunManager() override = default;

        /**
         * @brief Thread safe version of G4RunManager::BeamOn. Offload the work to a thread specific worker.
         * @param n_event number of events to simulate in one run.
         * @param seed1 First event seed for the worker run manager of the calling thread
         * @param seed2 Second event seed for the worker run manager of the calling thread
         *
         * Run the specified number of events on a separate worker that is associated with the calling thread.
         * The worker will be initialized with a new set of seeds to be used specifically for this event. The seeds are
         * obtained from the calling module, thereby ensuring that results can be reproduced.
         */
        void Run(G4int n_event, uint64_t seed1, uint64_t seed2); // NOLINT

        /**
         * @brief Initialize the run manager to be ready for run.
         *
         * Initializes the manager to be in a ready state. It will also prepare the random seeds which will be used
         * to seed the RNG on each worker thread.
         * If you want to set the seeds for G4 RNG it must happen before calling this method.
         */
        void Initialize() override;

        /**
         * @brief Initializes thread local objects including the worker manager.
         *
         * Initializes the worker run manager associated to the calling thread. This must be called by every thread
         * that intends to call \ref Run method. Only the first call by a given thread will actually initialize the
         * workers and further calls by the same thread will be ignored.
         */
        void InitializeForThread(); // NOLINT

        /**
         * @brief Cleanup worker specific data stored as thread local.
         *
         * Cleanup all thread local objects allocated previously by the calling thread. Each thread that ever used
         * this class must call this method to ensure correct termination.
         */
        void TerminateForThread(); // NOLINT

        /**
         * @brief Returns the user's sensitive detector construction.
         */
        SensitiveDetectorAndFieldConstruction* GetSDAndFieldConstruction() const { // NOLINT
            return sd_field_construction_.get();
        } // NOLINT

        /**
         * @brief Sets the user's sensitive detector construction.
         */
        void
        SetSDAndFieldConstruction(std::unique_ptr<SensitiveDetectorAndFieldConstruction> sd_field_construction) { // NOLINT
            sd_field_construction_ = std::move(sd_field_construction);
        }

    protected:
        /**
         * @brief Previously used by workers to wait for master commands.
         * Worker manager waits on the shared barrier until master issues a new command. It will now do nothing.
         */
        WorkerActionRequest ThisWorkerWaitForNextAction() override { return WorkerActionRequest::UNDEFINED; }

        /**
         * @brief Previously used to create threads and start worker managers.
         * Creates my own threads and start the worker run managers. It will now do nothing.
         */
        void CreateAndStartWorkers() override {}

        /**
         * @brief Previously used to issue a new command to the workers.
         * Send a new command to workers waiting for master to tell them what to do. It will now do nothing.
         */
        void NewActionRequest(WorkerActionRequest) override {}

        /**
         * @brief Previously used to generate initial seeds. We skip this since we provide seeds to each event directly
         * It is important to override this function to make sure Geant4 is not seeding itself independently.
         */
        G4bool InitializeSeeds(G4int) override { return true; };

        /**
         * @brief Previously used to tell workers to execute UI commands.
         * Send commands to workers to execute the UI commands stored in master. It will now do nothing.
         */
        void RequestWorkersProcessCommandsStack() override {}

        /**
         * @brief Previously used by the worker to initialize an event.
         *
         * Worker will ask us to setup the event. This includes figuring out the event number that worker will do
         * and setting up the seeds to ensure results can be reproduced. It will now do nothing.
         */
        G4bool SetUpAnEvent(G4Event*, long&, long&, long&, G4bool) override { return false; } // NOLINT

        /**
         * @brief Previously used by the worker to initialize N event.
         *
         * Worker will ask us to setup the events. This includes figuring out the event numbers that worker will do
         * and setting up the seeds to ensure results can be reproduced. It will now do nothing.
         */
        G4int SetUpNEvents(G4Event*, G4SeedsQueue*, G4bool) override { return 0; }

        /**
         * @brief Previously used to stop all the workers.
         * Stop the workers. It will now do nothing.
         */
        void TerminateWorkers() override {}

        /**
         * @brief Previously used by workers to signal they finished the event loop.
         * Synchronize with master about finishing the assigned work. It will now do nothing.
         */
        void ThisWorkerEndEventLoop() override {}

        /**
         * @brief Previously used by workers to signal they finished running UI commands.
         * Synchronize with master about finishing UI commands. It will now do nothing.
         */
        void ThisWorkerProcessCommandsStackDone() override {}

        /**
         * @brief Previously used by workers to signal they are ready to do work.
         * Synchronize with master we finished initialization and ready for work. It will now do nothing.
         */
        void ThisWorkerReady() override {}

        /**
         * @brief Previously used to wait until all workers have finished the event loop.
         * Wait for all the workers to finish and signal the end of event loop. It will now do nothing.
         */
        void WaitForEndEventLoopWorkers() override {}

        /**
         * @brief Previously used to wait for workers to finish initialization.
         * Wait for all the workers to finish initialization. It will now do nothing.
         */
        void WaitForReadyWorkers() override {}

    private:
        // \ref WorkerRunManager worker manager that run on each thread.
        static G4ThreadLocal WorkerRunManager* worker_run_manager_;

        std::unique_ptr<SensitiveDetectorAndFieldConstruction> sd_field_construction_{nullptr};
    };
} // namespace allpix

#endif /* ALLPIX_MT_RUN_MANAGER_H */
