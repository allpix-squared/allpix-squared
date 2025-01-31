/**
 * @file
 * @brief Defines the internal Geant4 geometry construction
 *
 * @copyright Copyright (c) 2019-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MODULE_DETECTOR_CONSTRUCTION_H
#define ALLPIX_MODULE_DETECTOR_CONSTRUCTION_H

#include <memory>
#include <utility>

#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4VSolid.hh"

#include "core/geometry/GeometryManager.hpp"

namespace allpix {
    /**
     * @brief Constructs the Geant4 geometry during Geant4 initialization
     */
    class DetectorConstructionG4 {
    public:
        /**
         * @brief Constructs geometry construction module
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         */
        explicit DetectorConstructionG4(GeometryManager* geo_manager);

        /**
         * @brief Constructs the world geometry with all detectors
         * @param world_log Shared pointer to the World logical volume
         */
        void build(const std::shared_ptr<G4LogicalVolume>& world_log);

    private:
        GeometryManager* geo_manager_;

        // Storage of internal objects
        std::vector<std::shared_ptr<G4VSolid>> solids_;
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_DETECTOR_CONSTRUCTION_H */
