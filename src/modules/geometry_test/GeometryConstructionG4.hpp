/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_DETECTOR_CONSTRUCTION_H
#define ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_DETECTOR_CONSTRUCTION_H

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "G4VUserDetectorConstruction.hh"
#include "G4ThreeVector.hh"
#include "G4Material.hh"

#include "../../core/geometry/GeometryManager.hpp"

//FIXME: improve this later on

namespace allpix {
    
    class GeometryConstructionG4 : public G4VUserDetectorConstruction {
    public:
        GeometryConstructionG4(GeometryManager *geo, G4ThreeVector world_size) : geo_manager_(geo), world_size_(world_size) {}
        ~GeometryConstructionG4() {};
        
        G4VPhysicalVolume* Construct();
        
    private:
        void BuildPixelDevices();
        
        // geometry manager
        GeometryManager *geo_manager_;
        
        // global input parameter for the world size (FIXME: determine this on the fly?)
        G4ThreeVector world_size_;
        
        // internal storage
        G4Material          *world_material_;
        G4LogicalVolume     *world_log_;
        G4VPhysicalVolume   *world_phys_;
    };
}

#endif /* ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_DETECTOR_CONSTRUCTION_H */
