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

class RunManager;

namespace allpix {
    /**
     * @brief Run manager for Geant4 that can be used by multiple threads where each thread will have its own instance.
     *
     * This manager overrides G4WorkerRunManager behaviour so it can be used on user defined threads. Therefore, there
     * is no dependency on the master run manager except only in initialization.
     */
    class WorkerRunManager : public G4WorkerRunManager {
        friend class RunManager;
    public:
        virtual ~WorkerRunManager();
    protected:
        WorkerRunManager() = default;
    };
} // namespace allpix

#endif /* ALLPIX_WORKER_RUN_MANAGER_H */
