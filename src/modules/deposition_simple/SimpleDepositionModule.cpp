/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 * @author Mathieu Benoit <benoit@lal.in2p3.fr>
 */

#include "SimpleDepositionModule.hpp"

#include "G4RunManager.hh"
#include "G4SDManager.hh"
#include "G4UImanager.hh"
#include "G4HadronicProcessStore.hh"
#include "G4ParticleDefinition.hh"

#include "GeneratorActionG4.hpp"
#include "SensitiveDetectorG4.hpp"

#include "../../tools/geant4.h"
#include "../../core/AllPix.hpp"
#include "../../core/geometry/GeometryManager.hpp"
#include "../../core/utils/log.h"

// FIXME: THIS IS BROKEN ---> SHOULD NEVER BE NEEDED !!!
#include "../geometry_test/DetectorModelG4.hpp"

using namespace allpix;

const std::string SimpleDepositionModule::name = "deposition_simple";

SimpleDepositionModule::SimpleDepositionModule(AllPix *apx, ModuleIdentifier id, Configuration config): Module(apx, id) {
    config_ = config;
}
SimpleDepositionModule::~SimpleDepositionModule() {}

// run the deposition
void SimpleDepositionModule::run() {
    LOG(INFO) << "INIT THE DEPOSITS";
    
    // load the G4 run manager from allpix
    std::shared_ptr<G4RunManager> run_manager_g4 = getAllPix()->getExternalManager<G4RunManager>();
    assert(run_manager_g4); // FIXME: temporary assert (throw a proper exception later if the manager is not defined)
    
    // add a generator
    // NOTE: for more difficult modules a separate generator module makes more sense probably?
    // FIXME: some random defaults
    config_.setDefault("particle_type", "e-");
    config_.setDefault("particle_amount", 1);
    config_.setDefault("particle_position", G4ThreeVector(-25, -25, 50));
    config_.setDefault("particle_momentum", G4ThreeVector(0,0,-1));
    config_.setDefault("particle_energy", 500.0);
    
    G4ParticleDefinition *particle = G4ParticleTable::GetParticleTable()->FindParticle(config_.get<std::string>("particle_type"));
    int part_amount = config_.get<int>("particle_amount");
    G4ThreeVector part_position = config_.get<G4ThreeVector>("particle_position");
    G4ThreeVector part_momentum = config_.get<G4ThreeVector>("particle_momentum");
    double part_energy = config_.get<double>("particle_energy");
    
    GeneratorActionG4 *generator = new GeneratorActionG4(part_amount, particle, part_position*CLHEP::um, part_momentum*CLHEP::um, part_energy*CLHEP::keV);
    run_manager_g4->SetUserAction(generator);
    
    // loop through all detectors and set the sensitive detectors
    //G4SDManager *sd_man_g4 = G4SDManager::GetSDMpointer(); //FIXME: do we need this sensitive detector manager
    for(auto &detector : getGeometryManager()->getDetectors()){
        auto sensitive_detector_g4 = new SensitiveDetectorG4(detector);
        
        //sd_man_g4->AddNewDetector(sensitive_detector_g4);
        detector->getExternalModel<DetectorModelG4>()->pixel_log->SetSensitiveDetector(sensitive_detector_g4);
    }
    
    // disable verbose processes
    // FIXME: there should be a more general way to do it, but I have not found it yet
    G4UImanager *UI = G4UImanager::GetUIpointer();
    UI->ApplyCommand("/process/verbose 0");
    UI->ApplyCommand("/process/em/verbose 0");
    UI->ApplyCommand("/process/eLoss/verbose 0");
    G4HadronicProcessStore::Instance()->SetVerbose(0);
    //run_manager_g4->RunInitialization();
    
    // start the beam
    LOG(INFO) << "START THE BEAM";
    run_manager_g4->BeamOn(config_.get("amount", 1));
    
    LOG(INFO) << "END DEPOSIT MODULE";
}


