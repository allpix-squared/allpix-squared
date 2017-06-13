/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_DETECTOR_CONSTRUCTION_H
#define ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_DETECTOR_CONSTRUCTION_H

#include <memory>
#include <utility>

#include "G4Material.hh"
#include "G4ThreeVector.hh"
#include "G4VSolid.hh"
#include "G4VUserDetectorConstruction.hh"

#include "core/geometry/GeometryManager.hpp"

#include "DetectorModelG4.hpp"

// FIXME: improve this later on

class G4UserLimits;

namespace allpix {

    class GeometryConstructionG4 : public G4VUserDetectorConstruction {
    public:
        // Constructor and destructor
        GeometryConstructionG4(GeometryManager* geo, const G4ThreeVector& world_size);
        ~GeometryConstructionG4() override;

        // Disallow copy
        GeometryConstructionG4(const GeometryConstructionG4&) = delete;
        GeometryConstructionG4& operator=(const GeometryConstructionG4&) = delete;

        // Default move
        GeometryConstructionG4(GeometryConstructionG4&&) = default;
        GeometryConstructionG4& operator=(GeometryConstructionG4&&) = default;

        // Construct the world
        G4VPhysicalVolume* Construct() override;

    private:
        void init_materials();
        void build_pixel_devices();

        GeometryManager* geo_manager_;

        // global input parameter for the world size (FIXME: determine this on the fly?)
        G4ThreeVector world_size_;

        // storage of all the internal Geant4 detectors
        std::vector<std::unique_ptr<DetectorModelG4>> models_;

        std::map<std::string, G4Material*> materials_;

        // internal storage
        std::vector<std::shared_ptr<G4VSolid>> solids_;
        G4Material* world_material_;
        std::unique_ptr<G4LogicalVolume> world_log_;
        std::unique_ptr<G4VPhysicalVolume> world_phys_;
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_DETECTOR_CONSTRUCTION_H */
