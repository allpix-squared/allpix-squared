#include "GeometryConstructionModule.hpp"

#include <vector>
#include <stdio.h>
#include <utility>
#include <string>
#include <map>
#include <cassert>

#include "G4RunManager.hh"
#include "G4PhysListFactory.hh"

#include "ReadGeoDescription.hpp"
#include "GeometryConstructionG4.hpp"
#include "DetectorModelG4.hpp"

#include "../../core/AllPix.hpp"
#include "../../core/geometry/GeometryManager.hpp"
#include "../../core/utils/log.h"

using namespace allpix;

// name of the module
const std::string GeometryConstructionModule::name = "geometry_test";

// constructor and destructor (defined here to allow for incomplete unique_ptr type)
GeometryConstructionModule::GeometryConstructionModule(AllPix *apx, ModuleIdentifier id, Configuration config): Module(apx, id) {
    config_ = config;
}
GeometryConstructionModule::~GeometryConstructionModule() {}

// run the geometry construction
void GeometryConstructionModule::run(){
    LOG(INFO) << "START BUILD GEOMETRY";
    
    // FIXME: check that geometry is empty or clean it before continuing
    
    // read the geometry
    std::string file_name = config_.get<std::string>("file");
    auto geo_descriptions = ReadGeoDescription(file_name);
    
    // build the detectors_
    // FIXME: hardcoded for now
    auto detector_model = geo_descriptions.GetDetectorsMap()[config_.get<std::string>("detector_name", "test")];  
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

/*void GeometryConstructionModule::clean(){
 *    // Clean geometry manager 
 *    getGeometryManager()->clear();
 *    
 *    // Clean G4 old geometry
 *    G4GeometryManager::GetInstance()->OpenGeometry();
 *    G4PhysicalVolumeStore::GetInstance()->Clean();
 *    G4LogicalVolumeStore::GetInstance()->Clean();
 *    G4SolidStore::GetInstance()->Clean();
 * }*/

void GeometryConstructionModule::buildG4() {
    // create the G4 run manager
    std::shared_ptr<G4RunManager> run_manager_g4 = std::make_shared<G4RunManager>();
    
    // UserInitialization classes - mandatory;
    // FIXME: allow direct parsing of vectors
    config_.setDefault("world_size", "1000 1000 2000");
    std::vector<int> world_size_arr = config_.getArray<int>("world_size");
    G4ThreeVector world_size(world_size_arr[0], world_size_arr[1], world_size_arr[2]);
    
    // set the geometry constructor
    GeometryConstructionG4 *geometry_construction = new GeometryConstructionG4(getGeometryManager(), world_size);
    run_manager_g4->SetUserInitialization(geometry_construction);
    
    // set the physics list
    // FIXME: set a good default physics list
    config_.setDefault("physics_list", "QGSP_BERT");
    G4PhysListFactory physListFactory;
    G4VUserPhysicsList *physicsList = physListFactory.GetReferencePhysList(config_.get<std::string>("physics_list"));
    run_manager_g4->SetUserInitialization(physicsList);
    
    // run the construct function in GeometryConstructionG4
    run_manager_g4->Initialize();
    
    // save the geant4 run manager in allpix to make it available to other modules
    getAllPix()->setExternalManager(run_manager_g4);
}
