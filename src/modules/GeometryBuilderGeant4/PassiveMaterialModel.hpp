/**
 * @file
 * @brief Base of passive material volumes
 *
 * @copyright Copyright (c) 2019-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

/**
 * @defgroup PassiveMaterialModels passive material models
 * @brief Collection of passive material models supported by the framework
 */

#ifndef ALLPIX_MODULE_PASSIVE_MATERIAL_MODEL_H
#define ALLPIX_MODULE_PASSIVE_MATERIAL_MODEL_H

#include "core/config/Configuration.hpp"
#include "core/config/exceptions.h"
#include "core/geometry/GeometryManager.hpp"
#include "core/utils/log.h"
#include "tools/ROOT.h"

#include <G4LogicalVolume.hh>
#include <G4RotationMatrix.hh>
#include <G4VSolid.hh>

namespace allpix {
    /**
     * @ingroup PassiveMaterialModels
     * @brief Base of all passive material models
     *
     * Creates the functions that will be overwritten by the passive material models.
     * The G4VSolid of the passive material and it's optional filling material are defined.
     * The maximum size parameter is defined.
     */
    class PassiveMaterialModel {
    public:
        /**
         * @brief Factory to dynamically create track objects
         * @param config Configuration with description of the model
         * @param geo_manager Pointer to the global geometry manager
         * @return By param trackModel assigned track model to be used
         */
        static std::shared_ptr<PassiveMaterialModel> factory(const Configuration& config, GeometryManager* geo_manager);

        /**
         * @brief Constructs the base passive material model
         * @param config Configuration with description of the model
         * @param geo_manager Pointer to the global geometry manager
         */
        PassiveMaterialModel(const Configuration& config, GeometryManager* geo_manager);

        /**
         * @brief Essential virtual destructor
         */
        virtual ~PassiveMaterialModel() = default;

        /**
         * @brief Virtual function that will return the maximum size parameter of the model
         * @return Maximum size of the model
         */
        virtual double getMaxSize() const = 0;

        virtual void buildVolume(const std::shared_ptr<G4LogicalVolume>& world_log);

        /**
         * @brief return name of this volume
         * @return Volume name
         */
        const std::string& getName() const { return name_; }

        /**
         * @brief return name of the mother volume or an empty string of none is set
         * @return Mother volume
         */
        const std::string& getMotherVolume() const { return mother_volume_; }

    protected:
        /**
         * @brief Virtual function that will return a G4VSolid corresponding to the specific model
         * @return Shared pointer to the model solid
         */
        virtual std::shared_ptr<G4VSolid> get_solid() const = 0;

        /**
         * @brief Set visualization attributes of the passive material as specified in the configuration
         * @param volume         Volume to set the visualization attributes for
         * @param mother_volume  Mother volume of the one in question
         */
        void set_visualization_attributes(G4LogicalVolume* volume, G4LogicalVolume* mother_volume);

        /**
         * @brief Delivers the points which represent the outer corners of the passive material to the GeometryManager
         */
        void add_points();

        Configuration config_;
        GeometryManager* geo_manager_;
        double max_size_{};

        std::string name_;
        ROOT::Math::Rotation3D orientation_;
        ROOT::Math::XYZPoint position_;
        std::shared_ptr<G4RotationMatrix> rotation_;
        std::string mother_volume_;

        // Storage of internal objects
        std::vector<std::shared_ptr<G4VSolid>> solids_;
    };
} // namespace allpix

#endif // ALLPIX_MODULE_PASSIVE_MATERIAL_MODEL_H
