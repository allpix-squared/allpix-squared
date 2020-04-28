/**
 * @file
 * @brief Defines the internal Geant4 passive material construction
 * @copyright Copyright (c) 2017-2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MODULE_PASSIVE_MATERIAL_CONSTRUCTION_H
#define ALLPIX_MODULE_PASSIVE_MATERIAL_CONSTRUCTION_H

#include <memory>
#include <utility>
#include <vector>

#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "PassiveMaterialModel.hpp"
#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"

namespace allpix {
    /**
     * @brief Constructs passive materials during Geant4 initialization
     */
    class PassiveMaterialConstructionG4 {
    public:
        /**
         * @brief Constructs passive material construction module
         */
        PassiveMaterialConstructionG4(GeometryManager* geo_manager);

        /**
         * @brief Constructs the passive materials
         * @return Physical volume representing the passive materials
         */
        void build(std::map<std::string, G4Material*> materials_);
        /**
         * @brief Defines the points which represent the outer corners of the passive material
         * @return Vector of all XYZ points of the corners
         */
        std::vector<ROOT::Math::XYZPoint> addPoints();

    private:
        GeometryManager* geo_manager_;

        // Storage of internal objects
        std::vector<ROOT::Math::XYZPoint> points_;
        std::vector<std::shared_ptr<G4VSolid>> solids_;
    };

} // namespace allpix

#endif /* ALLPIX_MODULE_PASSIVE_MATERIAL_CONSTRUCTION_H */
