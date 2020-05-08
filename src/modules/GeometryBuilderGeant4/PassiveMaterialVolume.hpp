/**
 * @file
 * @brief Carries the configurations of individual passive materials
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MODULE_PASSIVE_MATERIAL_VOLUME_H
#define ALLPIX_MODULE_PASSIVE_MATERIAL_VOLUME_H

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
     * @brief Represents one passive material volume
     */
    class PassiveMaterialVolume {
    public:
        /**
         * @brief Constructor
         */
        PassiveMaterialVolume(Configuration config, GeometryManager* geo_manager);

        void registerVolume();

        void buildVolume(const std::map<std::string, G4Material*>& materials, std::shared_ptr<G4LogicalVolume> world_log);

        /**
         * @brief Delivers the points which represent the outer corners of the passive material to the GeometryManager
         */
        void addPoints();

    private:
        std::string name_;
        std::string type_;
        std::shared_ptr<PassiveMaterialModel> model_;
        ROOT::Math::Rotation3D orientation_;
        ROOT::Math::XYZPoint position_;
        std::shared_ptr<G4RotationMatrix> rotation_;
        std::string mother_volume_;

        Configuration config_;
        GeometryManager* geo_manager_;

        // Storage of internal objects
        std::vector<std::shared_ptr<G4VSolid>> solids_;
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_PASSIVE_MATERIAL_VOLUME_H */
