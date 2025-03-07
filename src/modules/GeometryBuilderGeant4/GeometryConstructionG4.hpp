/**
 * @file
 * @brief Defines the internal Geant4 geometry construction
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_H
#define ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_H

#include <memory>
#include <utility>

#include <G4Material.hh>
#include <G4VSolid.hh>
#include <G4VUserDetectorConstruction.hh>

#include "DetectorConstructionG4.hpp"
#include "PassiveMaterialConstructionG4.hpp"
#include "core/geometry/GeometryManager.hpp"

namespace allpix {
    /**
     * @brief Constructs the Geant4 geometry during Geant4 initialization
     */
    class GeometryConstructionG4 : public G4VUserDetectorConstruction {

    public:
        /**
         * @brief Constructs geometry construction module
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         * @param config Configuration object of the geometry builder module
         */
        GeometryConstructionG4(GeometryManager* geo_manager, Configuration& config);

        /**
         * @brief Constructs the world geometry with all detectors
         * @return Physical volume representing the world
         */
        G4VPhysicalVolume* Construct() override;

    private:
        GeometryManager* geo_manager_;
        Configuration& config_;

        /**
         * @brief Check all placed volumes for overlaps
         */
        void check_overlaps() const;

        /**
         * @brief Verify that framework coordinate transformations match with the transformations built from Geant4 volumes
         */
        void verify_transforms() const;

        std::unique_ptr<DetectorConstructionG4> detector_builder_;
        std::unique_ptr<PassiveMaterialConstructionG4> passive_builder_;

        // Storage of internal objects
        std::vector<std::shared_ptr<G4VSolid>> solids_;

        std::shared_ptr<G4LogicalVolume> world_log_;
        std::unique_ptr<G4VPhysicalVolume> world_phys_;
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_H */
