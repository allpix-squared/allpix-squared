/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_DETECTOR_CONSTRUCTION_H
#define ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_DETECTOR_CONSTRUCTION_H

#include "G4Material.hh"
#include "G4ThreeVector.hh"
#include "G4VUserDetectorConstruction.hh"

#include "core/geometry/GeometryManager.hpp"

// FIXME: improve this later on

class G4UserLimits;

namespace allpix {

    class GeometryConstructionG4 : public G4VUserDetectorConstruction {
    public:
        // Constructor and destructor
        GeometryConstructionG4(GeometryManager* geo, G4ThreeVector world_size, bool simple_view);
        ~GeometryConstructionG4() override;

        // Disallow copy
        GeometryConstructionG4(const GeometryConstructionG4&) = delete;
        GeometryConstructionG4& operator=(const GeometryConstructionG4&) = delete;

        // Construct the world
        G4VPhysicalVolume* Construct() override;

    private:
        void build_pixel_devices();

        // geometry manager
        GeometryManager* geo_manager_;

        // global input parameter for the world size (FIXME: determine this on the fly?)
        G4ThreeVector world_size_;
        // determine if we have to build with simplified visualization (speed up)
        bool simple_view_;

        // internal storage
        G4Material* world_material_;
        G4LogicalVolume* world_log_;
        G4VPhysicalVolume* world_phys_;
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_DETECTOR_CONSTRUCTION_H */
