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
     * @brief Represents one passive material volume
     */
    class PassiveMaterialVolume {
    public:
        /**
         * @brief Constructor
         */
        PassiveMaterialVolume(const Configuration config, GeometryManager* geo_manager);

        void register_volume();

        void build_volume(std::map<std::string, G4Material*> materials);

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
         * @brief Registers the passive material
         */
        void register_volumes();

        /**
         * @brief Constructs the passive materials
         */
        void build_volumes(std::map<std::string, G4Material*> materials);

    private:
        GeometryManager* geo_manager_;
        std::vector<PassiveMaterialVolume*> passive_volumes_;
        std::vector<std::string> passive_volume_names_;
    };

} // namespace allpix

#endif /* ALLPIX_MODULE_PASSIVE_MATERIAL_CONSTRUCTION_H */
