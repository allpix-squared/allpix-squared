#include "GeometryConstructionModule.hpp"

#include "ReadGeoDescription.hpp"

#include "../../core/geometry/GeometryManager.hpp"
#include "../../core/utils/log.h"

#include "G4SDManager.hh"
#include "G4NistManager.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4PVDivision.hh"
#include "G4VSolid.hh"
#include "G4ThreeVector.hh"
#include "G4Box.hh"
#include "G4PVPlacement.hh"

#include "CLHEP/Units/SystemOfUnits.h"

#include <vector>
#include <stdio.h>
#include <utility>
#include <string>
#include <map>

using namespace CLHEP;
using namespace allpix;

// name of the module
const std::string GeometryConstructionModule::name = "geometry_test";

void GeometryConstructionModule::run(){
    LOG(DEBUG) << "START RUN";
    
    std::string file_name = config_.get<std::string>("file");
    auto geo_descriptions = ReadGeoDescription(file_name);
    
    /*for(auto &geo_desc : ){
     *        std::cout << geo_desc.first << " " << geo_desc.second->GetHalfChipX() << std::endl;
}*/
    
    auto detector_model = geo_descriptions.GetDetectorsMap()["mimosa"];  
    Detector det1("name1", detector_model);
    getGeometryManager()->addDetector(det1);
    
    Detector det2("name2", detector_model);
    getGeometryManager()->addDetector(det1);
    
    LOG(DEBUG) << "END RUN";
}

/*void GeometryConstructionModule::clean(){
    // Clean geometry manager 
    getGeometryManager()->clear();
    
    // Clean G4 old geometry
    G4GeometryManager::GetInstance()->OpenGeometry();
    G4PhysicalVolumeStore::GetInstance()->Clean();
    G4LogicalVolumeStore::GetInstance()->Clean();
    G4SolidStore::GetInstance()->Clean();
}*/

void GeometryConstructionModule::buildG4() {
    // Materials
    // vacuum (FIXME: useless code, only here for later) 
    double z,a,density;
    G4Material *vacuum = new G4Material("Vacuum", z=1 , a=1.01*g/mole, density= 0.0001*g/cm3);
    delete vacuum;
    
    // air
    G4NistManager * nistman = G4NistManager::Instance();
    G4Material *air = nistman->FindOrBuildMaterial("G4_AIR");
    
    // FIXME: stick to air as world material now
    G4Material *world_material = air;
    
    LOG(DEBUG) << "Material of world: " << world_material->GetName();
    
    /////////////////////////////////////////
    // The experimental Hall.  World
    G4VisAttributes * invisibleVisAtt = new G4VisAttributes(G4Color(1.0, 0.65, 0.0, 0.1));
    //G4VisAttributes invisibleVisAtt(G4Color(1.0, 0.65, 0.0, 0.1));
    invisibleVisAtt->SetVisibility(false);
    invisibleVisAtt->SetForceSolid(false);
    
    // FIXME: proper support of arrays...
    std::vector<int> world_size = config_.getArray<int>("world_size");
    G4Box* expHall_box = new G4Box("World",
                                   world_size[0],
                                   world_size[1],
                                   world_size[2]);
    
    G4LogicalVolume *expHall_log
    = new G4LogicalVolume(expHall_box,
                          world_material,
                          "World",
                          0,0,0);
    expHall_log->SetVisAttributes ( invisibleVisAtt );
    
    G4PVPlacement *expHall_phys
    = new G4PVPlacement(0,
                        G4ThreeVector(0.,0.,0.),
                        expHall_log,
                        "World",
                        0x0,
                        false,
                        0);
        
                
    //CheckAllPixSetup();
    //BuildPixelDevices(*geoMap);
    
    /*// Report absolute center of Si wafers
    if(!m_absolutePosSiWafer.empty()) {
        G4cout << "Absolute position of the Si wafers. center(x,y,z) : " << G4endl;
        vector<G4ThreeVector>::iterator itr = m_absolutePosSiWafer.begin();
        G4int cntr = 0;
        for( ; itr != m_absolutePosSiWafer.end() ; itr++) {
            
            G4cout << "   device [" << cntr << "] : "
            << (*itr).getX()/mm << " , "
            << (*itr).getY()/mm << " , "
            << (*itr).getZ()/mm << " [mm]"
            << G4endl;
            cntr++;
        }
    }*/
    
    //return expHall_phys;
}
