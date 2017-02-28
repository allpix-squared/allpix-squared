#include "GeometryConstructionModule.hpp"

#include <vector>
#include <stdio.h>
#include <utility>
#include <string>
#include <map>
#include <cassert>

#include "G4RunManager.hh"
#include "G4PhysListFactory.hh"
#include "G4UIsession.hh"
#include "G4UIterminal.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4VisManager.hh"

#include "ReadGeoDescription.hpp"
#include "GeometryConstructionG4.hpp"
#include "DetectorModelG4.hpp"

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

void GeometryConstructionModule::run(){
    LOG(INFO) << "START BUILD GEOMETRY";
    
    // read the geometry
    std::string file_name = config_.get<std::string>("file");
    auto geo_descriptions = ReadGeoDescription(file_name);
    
    /*for(auto &geo_desc : ){
     *        std::cout << geo_desc.first << " " << geo_desc.second->GetHalfChipX() << std::endl;
}*/
    
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
    
    // ALERT: TEMPORARY EXECUTE MACRO (AND HANG EVERYTHING)
    if(config_.has("macro")){
        G4VisManager* visManager = new G4VisExecutive;
        visManager->Initialize();
        
        G4UIsession *session = new G4UIterminal();
        G4UImanager *UI = G4UImanager::GetUIpointer();
        UI->ApplyCommand("/control/execute "+config_.get<std::string>("macro"));
        session->SessionStart();
        delete session;
    }
    
    auto ptr = getGeometryManager()->getDetector("name1")->getExternalModel<DetectorModelG4>();
    
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
    g4_run_manager_ = std::make_unique<G4RunManager>();
    
    // UserInitialization classes - mandatory;
    // FIXME: allow direct parsing of vectors
    config_.setDefault("world_size", "1000 1000 2000");
    std::vector<int> world_size_arr = config_.getArray<int>("world_size");
    G4ThreeVector world_size(world_size_arr[0], world_size_arr[1], world_size_arr[2]);
    
    // set the geometry constructor
    GeometryConstructionG4 *geometry_construction = new GeometryConstructionG4(getGeometryManager(), world_size);
    g4_run_manager_->SetUserInitialization(geometry_construction);
    
    // set the physics list
    // FIXME: set a good default physics list
    config_.setDefault("physics_list", "QGSP_BERT");
    G4PhysListFactory physListFactory;
    G4VUserPhysicsList *physicsList = physListFactory.GetReferencePhysList(config_.get<std::string>("physics_list"));
    g4_run_manager_->SetUserInitialization(physicsList);
    
    // run the construct function in GeometryConstructionG4
    g4_run_manager_->Initialize();
        
    //return expHall_phys;
}
