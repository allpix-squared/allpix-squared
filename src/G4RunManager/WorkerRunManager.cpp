/**
 * @file
 * @brief Implementation of WorkerRunManager
 * @copyright Copyright (c) 2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "WorkerRunManager.hpp"
#include "MTRunManager.hpp"
#include "SensitiveDetectorAndFieldConstruction.hpp"

#include <atomic>

#include <G4UserWorkerInitialization.hh>
#include <G4UserRunAction.hh>
#include <G4VUserPrimaryGeneratorAction.hh>
#include <G4Threading.hh>
#include <G4WorkerRunManager.hh>
#include <G4MTRunManager.hh>
#include <G4UImanager.hh>
#include <G4UserWorkerThreadInitialization.hh>
#include <G4WorkerThread.hh>
#include <G4UserWorkerInitialization.hh>
#include <G4VUserActionInitialization.hh>
#include <G4TransportationManager.hh>

using namespace allpix;

namespace {
    // counter to give each worker a unique id
    static std::atomic<int> counter;
}

WorkerRunManager::~WorkerRunManager() {
    G4MTRunManager* master_run_manager = G4MTRunManager::GetMasterRunManager();

    // Step-6: Terminate worker thread
    if (master_run_manager->GetUserWorkerInitialization()) { 
        master_run_manager->GetUserWorkerInitialization()->WorkerStop();
    }
}

void WorkerRunManager::BeamOn(G4int n_event,const char* macroFile,G4int n_select)
{
    G4MTRunManager* mrm = G4MTRunManager::GetMasterRunManager();

    // G4Comment: Execute UI commands stored in the master UI manager
    std::vector<G4String> cmds = mrm->GetCommandStack();
    G4UImanager* uimgr = G4UImanager::GetUIpointer(); //TLS instance
    std::vector<G4String>::const_iterator it = cmds.begin();
    for (; it != cmds.end(); it++) {
        uimgr->ApplyCommand(*it);
    }

    G4RunManager::BeamOn(n_event, macroFile, n_select);
}

void WorkerRunManager::InitializeGeometry() {
    if(!userDetector)
    {
        G4Exception("WorkerRunManager::InitializeGeometry", "Run0033",
            FatalException, "G4VUserDetectorConstruction is not defined!");
        return;
    }
    if(fGeometryHasBeenDestroyed) 
    { G4TransportationManager::GetTransportationManager()->ClearParallelWorlds(); }

    //Step1: Get pointer to the physiWorld (note: needs to get the "super pointer, i.e. the one shared by all threads"
    G4RunManagerKernel* masterKernel = G4MTRunManager::GetMasterRunManagerKernel();
    G4VPhysicalVolume* worldVol = masterKernel->GetCurrentWorld();
    //Step2:, Call a new "WorkerDefineWorldVolume( pointer from 2-, false); 
    kernel->WorkerDefineWorldVolume(worldVol,false);
    kernel->SetNumberOfParallelWorld(masterKernel->GetNumberOfParallelWorld());
    //Step3: Call user's ConstructSDandField()
    MTRunManager* master_run_manager = static_cast<MTRunManager*>(G4MTRunManager::GetMasterRunManager());
    SensitiveDetectorAndFieldConstruction* detector_construction = master_run_manager->GetSDAndFieldConstruction();
    if (!detector_construction) {
        G4Exception("WorkerRunManager::InitializeGeometry", "Run0033",
            FatalException, "DetectorConstruction is not defined!");
        return;
    }
    detector_construction->ConstructSDandField();
    //userDetector->ConstructParallelSD();
    geometryInitialized = true;
}

void WorkerRunManager::DoEventLoop(G4int n_event,const char* macroFile,G4int n_select)
{
    if(!userPrimaryGeneratorAction)
    {
      G4Exception("WorkerRunManager::GenerateEvent()", "Run0032", FatalException,
                "G4VUserPrimaryGeneratorAction is not defined!");
    }

    InitializeEventLoop(n_event, macroFile, n_select);

    // For each run, worker should receive exactly one set of random number seeds.
    runIsSeeded = false; 

    // Event loop
    eventLoopOnGoing = true;
    G4int i_event = -1;
    nevModulo = -1;
    currEvID = -1;

    while(eventLoopOnGoing)
    {
      ProcessOneEvent(i_event);
      if(eventLoopOnGoing)
      {
        TerminateOneEvent();
        if(runAborted)
        { eventLoopOnGoing = false; }
      }
    }
     
    TerminateEventLoop();
}

G4Event* WorkerRunManager::GenerateEvent(G4int i_event)
{
    (void)i_event;
    if(!userPrimaryGeneratorAction)
    {
        G4Exception("WorkerRunManager::GenerateEvent()", "Run0032", FatalException,
                "G4VUserPrimaryGeneratorAction is not defined!");
        return nullptr;
    }

    G4Event* anEvent = nullptr;
    long s1, s2, s3;
    s1 = s2 = s3 = 0;

    if( numberOfEventProcessed < numberOfEventToBeProcessed && !runAborted ) {
        anEvent  = new G4Event(numberOfEventProcessed);

        if (!runIsSeeded) {
            // Seeds are stored in this queue to ensure we can reproduce the results of events
            // each event will reseed the random number generator
            s1 = seedsQueue.front(); seedsQueue.pop();
            s2 = seedsQueue.front(); seedsQueue.pop();

            // Seed RNG for this run only once
            long seeds[3] = { s1, s2, s3 };
            G4Random::setTheSeeds(seeds,-1);
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

WorkerRunManager* WorkerRunManager::GetNewInstanceForThread() {
    WorkerRunManager* thread_run_manager = nullptr;
    G4MTRunManager* master_run_manager = G4MTRunManager::GetMasterRunManager();

    // Step-0: Thread ID
    // Initliazie per-thread stream-output
    // The following line is needed before we actually do I/O initialization
    // because the constructor of UI manager resets the I/O destination.
    G4int this_id = counter.fetch_add(1);
    G4Threading::G4SetThreadId( this_id );
    G4UImanager::GetUIpointer()->SetUpForAThread( this_id );

    // Step-1: Random number engine
    // RNG Engine needs to be initialized by "cloning" the master one.
    const CLHEP::HepRandomEngine* master_engine = master_run_manager->getMasterRandomEngine();
    master_run_manager->GetUserWorkerThreadInitialization()->SetupRNGEngine(master_engine);

    // Step-2: Initialize worker thread
    if (master_run_manager->GetUserWorkerInitialization()) {
        master_run_manager->GetUserWorkerInitialization()->WorkerInitialize();
    }

    if(master_run_manager->GetUserActionInitialization()) {
        G4VSteppingVerbose* stepping_verbose = 
            master_run_manager->GetUserActionInitialization()->InitializeSteppingVerbose();
        if (stepping_verbose) {
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

    thread_run_manager->G4RunManager::SetUserInitialization(
        const_cast<G4VUserDetectorConstruction*>(detector));

    const G4VUserPhysicsList* physicslist = master_run_manager->GetUserPhysicsList();
    thread_run_manager->SetUserInitialization(const_cast<G4VUserPhysicsList*>(physicslist));

    // Step-4: Initialize worker run manager
    if (master_run_manager->GetUserActionInitialization()) {
        master_run_manager->GetNonConstUserActionInitialization()->Build();
    }

    if (master_run_manager->GetUserWorkerInitialization()) {
        master_run_manager->GetUserWorkerInitialization()->WorkerStart();
    }

    thread_run_manager->Initialize();

    // Execute UI commands stored in the masther UI manager
    std::vector<G4String> cmds = master_run_manager->GetCommandStack();
    G4UImanager* uimgr = G4UImanager::GetUIpointer(); //TLS instance
    std::vector<G4String>::const_iterator it = cmds.begin();
    for(; it != cmds.end(); it++)
    { 
        uimgr->ApplyCommand(*it);
    }

    return thread_run_manager;
}
