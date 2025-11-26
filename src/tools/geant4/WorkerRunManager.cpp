/**
 * @file
 * @brief Implementation of WorkerRunManager
 *
 * @copyright Copyright (c) 2019-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "WorkerRunManager.hpp"

#include <atomic>
#include <vector>

#include <CLHEP/Random/RandomEngine.h>
#include <G4ApplicationState.hh>
#include <G4MTRunManager.hh>
#include <G4RunManager.hh>
#include <G4RunManagerKernel.hh>
#include <G4StateManager.hh>
#include <G4String.hh>
#include <G4Threading.hh>
#include <G4TransportationManager.hh>
#include <G4UImanager.hh>
#include <G4UserWorkerInitialization.hh>
#include <G4UserWorkerThreadInitialization.hh>
#include <G4VUserActionInitialization.hh>
#include <G4VUserPrimaryGeneratorAction.hh>
#include <G4WorkerRunManager.hh>
#include <G4WorkerThread.hh>
#include <G4ios.hh>
#include <Randomize.hh>
#include <magic_enum/magic_enum.hpp>

#include "core/module/exceptions.h"
#include "core/utils/log.h"
#include "tools/geant4/G4ExceptionHandler.hpp"
#include "tools/geant4/G4LoggingDestination.hpp"
#include "tools/geant4/MTRunManager.hpp"
#include "tools/geant4/SensitiveDetectorAndFieldConstruction.hpp"

using namespace allpix;

namespace {
    // counter to give each worker a unique id
    std::atomic<int> counter;
} // namespace

WorkerRunManager::WorkerRunManager() {
    G4UImanager* ui_g4 = G4UImanager::GetUIpointer();
    ui_g4->SetCoutDestination(G4LoggingDestination::getInstance());
    // Set exception handler for Geant4 exceptions:
    G4StateManager::GetStateManager()->SetExceptionHandler(new G4ExceptionHandler());
}

WorkerRunManager::~WorkerRunManager() {
    G4MTRunManager* master_run_manager = G4MTRunManager::GetMasterRunManager();

    // Step-6: Terminate worker thread
    if(master_run_manager->GetUserWorkerInitialization() != nullptr) {
        master_run_manager->GetUserWorkerInitialization()->WorkerStop();
    }
}

void WorkerRunManager::BeamOn(G4int n_event, const char* macroFile, G4int n_select) { // NOLINT
    G4MTRunManager* mrm = G4MTRunManager::GetMasterRunManager();

    // G4Comment: Execute UI commands stored in the master UI manager
    std::vector<G4String> cmds = mrm->GetCommandStack();
    G4UImanager* uimgr = G4UImanager::GetUIpointer();        // TLS instance
    std::vector<G4String>::const_iterator it = cmds.begin(); // NOLINT
    for(; it != cmds.end(); it++) {
        uimgr->ApplyCommand(*it);
    }

    G4RunManager::BeamOn(n_event, macroFile, n_select);
}

void WorkerRunManager::InitializeGeometry() {
    if(userDetector == nullptr) {
        throw ModuleError("G4VUserDetectorConstruction is not defined!");
    }
    if(fGeometryHasBeenDestroyed) {
        G4TransportationManager::GetTransportationManager()->ClearParallelWorlds();
    }

    // Step1: Get pointer to the physiWorld (note: needs to get the "super pointer, i.e. the one shared by all threads"
    G4RunManagerKernel* masterKernel = G4MTRunManager::GetMasterRunManagerKernel();
    G4VPhysicalVolume* worldVol = masterKernel->GetCurrentWorld();
    // Step2:, Call a new "WorkerDefineWorldVolume( pointer from 2-, false);
    kernel->WorkerDefineWorldVolume(worldVol, false);
    kernel->SetNumberOfParallelWorld(masterKernel->GetNumberOfParallelWorld());
    // Step3: Call user's ConstructSDandField()
    auto* master_run_manager = dynamic_cast<MTRunManager*>(G4MTRunManager::GetMasterRunManager());
    SensitiveDetectorAndFieldConstruction* detector_construction = master_run_manager->GetSDAndFieldConstruction();
    if(detector_construction == nullptr) {
        throw ModuleError("DetectorConstruction is not defined!");
    }
    detector_construction->ConstructSDandField();
    // userDetector->ConstructParallelSD();
    geometryInitialized = true;
}

void WorkerRunManager::DoEventLoop(G4int n_event, const char* macroFile, G4int n_select) { // NOLINT
    if(userPrimaryGeneratorAction == nullptr) {
        throw ModuleError("G4VUserPrimaryGeneratorAction is not defined!");
    }

    InitializeEventLoop(n_event, macroFile, n_select);

    // For each run, worker should receive exactly one set of random number seeds.
    runIsSeeded = false;

    // Event loop
    eventLoopOnGoing = true;
    nevModulo = -1;
    currEvID = -1;
    while(eventLoopOnGoing) {
        ProcessOneEvent(-1);
        if(eventLoopOnGoing) {
            TerminateOneEvent();
            if(runAborted) {
                eventLoopOnGoing = false;
            }
        }
    }

    TerminateEventLoop();
}

G4Event* WorkerRunManager::GenerateEvent(G4int /*i_event*/) {
    if(userPrimaryGeneratorAction == nullptr) {
        throw ModuleError("G4VUserPrimaryGeneratorAction is not defined!");
    }

    G4Event* anEvent = nullptr;

    if(numberOfEventProcessed < numberOfEventToBeProcessed && !runAborted) {
        anEvent = new G4Event(numberOfEventProcessed);

        if(!runIsSeeded) {
            // Seeds are stored in this queue to ensure we can reproduce the results of events
            // each event will reseed the random number generator
            long const s1 = seedsQueue.front();
            seedsQueue.pop();
            long const s2 = seedsQueue.front();
            seedsQueue.pop();

            // Seed RNG for this run only once
            long seeds[3] = {s1, s2, 0}; // NOLINT
            G4Random::setTheSeeds(seeds, -1);
            runIsSeeded = true;
        }

        userPrimaryGeneratorAction->GeneratePrimaries(anEvent);
    } else {
        // This flag must be set so the event loop exits if no more events
        // to be processed
        eventLoopOnGoing = false;
    }

    return anEvent;
}

WorkerRunManager* WorkerRunManager::GetNewInstanceForThread() { // NOLINT
    WorkerRunManager* thread_run_manager = nullptr;
    G4MTRunManager* master_run_manager = G4MTRunManager::GetMasterRunManager();

    // Step-0: Thread ID
    // Initliazie per-thread stream-output
    // The following line is needed before we actually do I/O initialization
    // because the constructor of UI manager resets the I/O destination.
    G4int const this_id = counter.fetch_add(1);
    G4Threading::G4SetThreadId(this_id);
    G4UImanager::GetUIpointer()->SetUpForAThread(this_id);

    // Step-1: Random number engine
    // RNG Engine needs to be initialized by "cloning" the master one.
    const CLHEP::HepRandomEngine* master_engine = master_run_manager->getMasterRandomEngine();
    master_run_manager->GetUserWorkerThreadInitialization()->SetupRNGEngine(master_engine);

    // Step-2: Initialize worker thread
    if(master_run_manager->GetUserWorkerInitialization() != nullptr) {
        master_run_manager->GetUserWorkerInitialization()->WorkerInitialize();
    }

    if(master_run_manager->GetUserActionInitialization() != nullptr) {
        G4VSteppingVerbose* stepping_verbose =
            master_run_manager->GetUserActionInitialization()->InitializeSteppingVerbose();
        if(stepping_verbose != nullptr) {
            G4VSteppingVerbose::SetInstance(stepping_verbose);
        }
    }

    // Now initialize worker part of shared objects (geometry/physics)
    G4WorkerThread::BuildGeometryAndPhysicsVector();

    // create the new instance
    thread_run_manager = new WorkerRunManager;

    // Step-3: Setup worker run manager
    // Set the detector and physics list to the worker thread. Share with master
    const G4VUserDetectorConstruction* detector = master_run_manager->GetUserDetectorConstruction();

    thread_run_manager->G4RunManager::SetUserInitialization(const_cast<G4VUserDetectorConstruction*>(detector)); // NOLINT

    const G4VUserPhysicsList* physicslist = master_run_manager->GetUserPhysicsList();
    thread_run_manager->SetUserInitialization(const_cast<G4VUserPhysicsList*>(physicslist)); // NOLINT

    // Step-4: Initialize worker run manager
    if(master_run_manager->GetUserActionInitialization() != nullptr) {
        master_run_manager->GetNonConstUserActionInitialization()->Build();
    }

    if(master_run_manager->GetUserWorkerInitialization() != nullptr) {
        master_run_manager->GetUserWorkerInitialization()->WorkerStart();
    }

    thread_run_manager->Initialize();

    // Execute UI commands stored in the master UI manager
    std::vector<G4String> cmds = master_run_manager->GetCommandStack();
    G4UImanager* uimgr = G4UImanager::GetUIpointer();        // TLS instance
    std::vector<G4String>::const_iterator it = cmds.begin(); // NOLINT
    for(; it != cmds.end(); it++) {
        uimgr->ApplyCommand(*it);
    }

    return thread_run_manager;
}

void WorkerRunManager::AbortRun(bool softAbort) {
    // This method is valid only for GeomClosed or EventProc state
    G4ApplicationState const currentState = G4StateManager::GetStateManager()->GetCurrentState();
    if(currentState == G4State_GeomClosed || currentState == G4State_EventProc) {
        runAborted = true;
        if(currentState == G4State_EventProc && !softAbort) {
            currentEvent->SetEventAborted();
            eventManager->AbortCurrentEvent();
            LOG(TRACE) << "Aborted Geant4 event";
        }
        // Ready for new event, set the state back to G4State_Idle
        G4StateManager::GetStateManager()->SetNewState(G4State_Idle);
        LOG(DEBUG) << "Reset Geant4 state to "
                   << magic_enum::enum_name(G4StateManager::GetStateManager()->GetCurrentState());
    } else {

        LOG(WARNING) << "Run is not in progress. AbortRun() ignored." << G4endl;
    }
}
