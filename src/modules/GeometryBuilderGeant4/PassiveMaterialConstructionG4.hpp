/**
 * @file
 * @brief Wrapper for the Geant4 Passive Material Construction
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MODULE_PASSIVE_MATERIAL_CONSTRUCTION_H
#define ALLPIX_MODULE_PASSIVE_MATERIAL_CONSTRUCTION_H

#include <vector>

#include <G4LogicalVolume.hh>
#include <G4Material.hh>

#include "core/geometry/GeometryManager.hpp"

#include "PassiveMaterialModel.hpp"

namespace allpix {
    /**
     * @brief Constructs passive materials during Geant4 initialization
     */
    class PassiveMaterialConstructionG4 {
    public:
        /**
         * @brief Constructs passive material construction module
         */
        explicit PassiveMaterialConstructionG4(GeometryManager* geo_manager);

        /**
         * @brief Registers the passive material
         */
        void registerVolumes();

        /**
         * @brief Constructs the passive materials
         */
        void buildVolumes(const std::shared_ptr<G4LogicalVolume>& world_log);

    private:
        GeometryManager* geo_manager_;
        std::vector<std::shared_ptr<PassiveMaterialModel>> passive_volumes_;
    };

} // namespace allpix

#endif /* ALLPIX_MODULE_PASSIVE_MATERIAL_CONSTRUCTION_H */
