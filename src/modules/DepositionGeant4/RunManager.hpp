/**
 * @file
 * @brief The RunManager class, defines a custom Geant4 RunManager that works with Allpix threads.
 * @copyright Copyright (c) 2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_RUN_MANAGER_H
#define ALLPIX_RUN_MANAGER_H

#include <G4MTRunManager.hh>

namespace allpix {

    class WorkerRunManager;

    /**
     * @brief A custom run manager for Geant4 that can work with external threads and be used concurrently.
     *
     * This manager overrides G4MTRunManager class so it doesn't create its own thread and work with the threads
     * already created by \ref ModuleManager class. Also, it provides a concurrent API that can be used by
     * multiple threads safely at the same time.
     * Most of the APIs defiend by \ref G4MTRunManager are overriden to simply do nothing since this custom run
     * manager doesn't operate its own event loop and assumes it is part of the client event loop and the results
     * of each event are independet from each other. Also, this  manager doesn't maintain any threads, it only
     * maintains the worker managers which are allocated on a per thread basis.
     */
    class RunManager : public G4MTRunManager {
        friend class WorkerRunManager;
    public:
        virtual ~RunManager() = default;

        /**
         * @brief Initialize the run manager to be ready for run.
         *
         * Initializes the manager to be in a ready state. It will also prepare the random seeds which will be used
         * to seed the RNG on each worker thread.
         * If you want to set the seeds for G4 RNG it must happen before calling this method.
         */
        virtual void Initialize() override;

    protected:
        RunManager() = default;

        /**
         * @brief Previously used by workers to wait for master commands.
         *
         * Worker manager waits on the shared barrier untill master issues a new command. It will now do nothing.
         */
        virtual WorkerActionRequest ThisWorkerWaitForNextAction() override {
            return WorkerActionRequest::UNDEFINED;
        }

        /**
         * @brief Previously used to create threads and start worker managers.
         *
         * Creates my own threads and start the worker run managers. It will now do nothing.
         */
        virtual void CreateAndStartWorkers() override {}

        /**
         * @brief Previously used to issue a new command to the workers.
         *
         * Send a new command to workers waiting for master to tell them what to do. It will now do nothing.
         */        
        virtual void NewActionRequest( WorkerActionRequest ) override {}

        /**
         * @brief Previously used to tell workers to execute UI commands.
         *
         * Send commands to workers to execute the UI commands stored in master. It will now do nothing.
         */
        virtual void RequestWorkersProcessCommandsStack() override {}

        /**
         * @brief Previously used by the worker to initialize an event.
         *
         * Worker will ask us to setup the event. This includes figuring out the event number that worker will do
         * and setting up the seeds to ensure results can be reproduced. It will now do nothing.
         */
        virtual G4bool SetUpAnEvent(G4Event*, long&, long&, long&, G4bool) override {
            return false;
        }

        /**
         * @brief Previously used by the worker to initialize N event.
         *
         * Worker will ask us to setup the events. This includes figuring out the event numbers that worker will do
         * and setting up the seeds to ensure results can be reproduced. It will now do nothing.
         */
        virtual G4int SetUpNEvents(G4Event*, G4SeedsQueue*, G4bool) override {
            return 0;
        }

        /**
         * @brief Previously used to stop all the workers.
         *
         * Stop the workers. It will now do nothing.
         */
        virtual void TerminateWorkers() override {}

        /**
         * @brief Previously used by workers to signal they finished the event loop.
         *
         * Synchronize with master about finishing the assigned work. It will now do nothing.
         */
        virtual void ThisWorkerEndEventLoop() override {}

        /**
         * @brief Previously used by workers to signal they finished running UI commands.
         *
         * Synchronize with master about finishing UI commands. It will now do nothing.
         */
        virtual void ThisWorkerProcessCommandsStackDone() override {}

        /**
         * @brief Previously used by workers to signal they are ready to do work.
         *
         * Synchronize with master we finished initialization and ready for work. It will now do nothing.
         */
        virtual void ThisWorkerReady() override {}

        /**
         * @brief Previously used to wait untill all workers have finished the event loop.
         *
         * Wait for all the workers to finish and signal the end of event loop. It will now do nothing.
         */
        virtual void WaitForEndEventLoopWorkers() override {}

        /**
         * @brief Previously used to wait for workers to finish initialization.
         *
         * Wait for all the workers to finish initialization. It will now do nothing.
         */
        virtual void WaitForReadyWorkers() override {}
    
    private:
        // \ref WorkerRunManager worker manager that run on each thread.
        static G4ThreadLocal WorkerRunManager* worker_run_manager_; 
    };
} // namespace allpix

#endif /* ALLPIX_RUN_MANAGER_H */
