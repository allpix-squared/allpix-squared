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

class WorkerRunManager;

namespace allpix {
    /**
     * @brief A custom run manager for Geant4 that can work with external threads and be used concurrently.
     *
     * This manager overrides G4MTRunManager class so it doesn't create its own thread and work with the threads
     * already created by \ref ModuleManager class. Also, it provides a concurrent API that can be used by
     * multiple threads safely at the same time.
     */
    class RunManager : public G4MTRunManager {
        friend class WorkerRunManager;
    public:
        virtual ~RunManager() = default;
    protected:
        RunManager() = default;
    };
} // namespace allpix

#endif /* ALLPIX_RUN_MANAGER_H */
