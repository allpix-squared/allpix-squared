#include "GeometryConstructionModule.hpp"

#include <vector>
#include <stdio.h>
#include <utility>
#include <string>
#include <map>
#include <cassert>

#include "G4RunManager.hh"
#include "G4PhysListFactory.hh"

#include "G4VisExecutive.hh"
#include "G4VisManager.hh"
#include "G4UImanager.hh"
#include "G4UIterminal.hh"

#include "ReadGeoDescription.hpp"
#include "GeometryConstructionG4.hpp"
#include "DetectorModelG4.hpp"

#include "../../tools/geant4.h"

#include "../../core/AllPix.hpp"
#include "../../core/geometry/GeometryManager.hpp"
#include "../../core/utils/log.h"
#include "../../core/utils/exceptions.h"

using namespace allpix;

// name of the module
const std::string GeometryConstructionModule::name = "geometry_test";

// constructor and destructor (defined here to allow for incomplete unique_ptr type)
GeometryConstructionModule::GeometryConstructionModule(AllPix *apx, ModuleIdentifier id, Configuration config): 
        Module(apx, id), config_(config), run_manager_g4_(nullptr) {}
GeometryConstructionModule::~GeometryConstructionModule() {}

// create the run manager and make it available
void GeometryConstructionModule::init(){
    // suppress all output (also cout due to a part in Geant4 where G4cout is not used)
    SUPPRESS_STREAM(std::cout);
    SUPPRESS_STREAM(G4cout);
    
    // create the G4 run manager
    run_manager_g4_ = std::make_shared<G4RunManager>();
    
    // release the output again
    RELEASE_STREAM(std::cout);
    RELEASE_STREAM(G4cout);
    
    // save the geant4 run manager in allpix to make it available to other modules
    getAllPix()->setExternalManager(run_manager_g4_);  
}

// run the geometry construction
void GeometryConstructionModule::run(){
    LOG(INFO) << "START BUILD GEOMETRY";
    
    // FIXME: check that geometry is empty or clean it before continuing
    
    // read the geometry
    std::string file_name = config_.get<std::string>("file");
    auto geo_descriptions = ReadGeoDescription(file_name);
    
    // build the detectors_
    // FIXME: hardcoded for now
    auto detector_model = geo_descriptions.getDetectorModel(config_.get<std::string>("detector_name", "test")); //geo_descriptions.GetDetectorsMap()[];  
    assert(detector_model); // FIXME: temporary assert
    
    auto det1 = std::make_shared<Detector>("name1", detector_model);
    getGeometryManager()->addDetector(det1);
    
    //Detector det2("name2", detector_model);
    //getGeometryManager()->addDetector(det2);
    
    // construct the G4 geometry
    buildG4();
    
    // finish
    LOG(INFO) << "END BUILD GEOMETRY";
}

void GeometryConstructionModule::buildG4() {
    // suppress all output for G4
    SUPPRESS_STREAM(G4cout);
    
    // get the world size
    config_.setDefault("world_size", G4ThreeVector(1000, 1000, 2000));
    G4ThreeVector world_size = config_.get<G4ThreeVector>("world_size");
    
    // set the geometry constructor
    GeometryConstructionG4 *geometry_construction = new GeometryConstructionG4(getGeometryManager(), world_size);
    run_manager_g4_->SetUserInitialization(geometry_construction);
    
    // set the physics list
    // FIXME: set a good default physics list
    config_.setDefault("physics_list", "QGSP_BERT");
    G4PhysListFactory physListFactory;
    G4VUserPhysicsList *physicsList = physListFactory.GetReferencePhysList(config_.get<std::string>("physics_list"));
    if(physicsList == nullptr){
        // FIXME: better syntax for exceptions here
        // FIXME: more information about available lists
        throw InvalidValueError("physics_list", config_.getName(), config_.getText("physics_list"), "physics list is not defined");
    }
    run_manager_g4_->SetUserInitialization(physicsList);
    
    // run the construct function in GeometryConstructionG4
    run_manager_g4_->Initialize();
    
    // release output from G4
    RELEASE_STREAM(G4cout);
}
