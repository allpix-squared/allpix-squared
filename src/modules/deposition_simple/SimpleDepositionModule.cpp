/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 * @author Mathieu Benoit <benoit@lal.in2p3.fr>
 */

#include "SimpleDepositionModule.hpp"

#include "G4RunManager.hh"
#include "G4SDManager.hh"

#include "GeneratorActionG4.hpp"
#include "SensitiveDetectorG4.hpp"

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
    // FIXME: it makes more sense to have a different set of modules to generate the events maybe?
    GeneratorActionG4 *generator = new GeneratorActionG4;
    run_manager_g4->SetUserAction(generator);
    
    // loop through all detectors and set the sensitive detectors
    G4SDManager *sd_man_g4 = G4SDManager::GetSDMpointer();
    for(auto &detector : getGeometryManager()->getDetectors()){
        auto sensitive_detector_g4 = new SensitiveDetectorG4(detector);
        
        sd_man_g4->AddNewDetector(sensitive_detector_g4);
        detector->getExternalModel<DetectorModelG4>()->pixel_log->SetSensitiveDetector(sensitive_detector_g4);
    }
    
    // start the beam
    LOG(INFO) << "START THE BEAM";
    run_manager_g4->BeamOn(1);
    
    LOG(INFO) << "END DEPOSIT MODULE";
}


