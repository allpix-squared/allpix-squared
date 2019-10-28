/**
 * @file
 * @brief Parameters of a box passive material model
 *
 * @copyright Copyright (c) 2017-2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_PASSIVE_MATERIAL_BOX_H
#define ALLPIX_PASSIVE_MATERIAL_BOX_H

#include <string>
#include <utility>

#include <Math/Cartesian2D.h>
#include <Math/DisplacementVector2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

#include <G4Box.hh>
#include <G4SubtractionSolid.hh>

#include "PassiveMaterialModel.hpp"

namespace allpix {

    /**
     * @ingroup PassiveMaterialModel
     * @brief Model of a rectangular box. The box can be filled with a filling material.
     */
    class BoxModel : public PassiveMaterialModel {
    public:
        /**
         * @brief Constructs the box passive material model
         * @param config Configuration with description of the model
         */
        explicit BoxModel(Configuration& config) : PassiveMaterialModel(config), config_(config) {

            // Set the box specifications
            setSize(config.get<ROOT::Math::XYZVector>("size"));
            setFillingSize(config.get<ROOT::Math::XYZVector>("filling_size", ROOT::Math::XYZVector()));
            std::string name = config_.getName();

            // Limit the values that can be given
            if(inner_size_.x() >= outer_size_.x() || inner_size_.y() >= outer_size_.y() ||
               inner_size_.z() >= outer_size_.z()) {
                throw InvalidValueError(config_, "inner_size", "inner_size cannot be larger than the outer_size");
            }

            // Create the G4VSolids which make the box
            auto outer_volume =
                new G4Box(name + "outer_volume", outer_size_.x() / 2, outer_size_.y() / 2, outer_size_.z() / 2);
            auto inner_volume =
                new G4Box(name + "inner_volume", inner_size_.x() / 2, inner_size_.y() / 2, inner_size_.z() / 2);

            solid_ = new G4SubtractionSolid(name + "_volume", outer_volume, inner_volume);
            filling_solid_ =
                new G4Box(name + "filling_volume", inner_size_.x() / 2, inner_size_.y() / 2, inner_size_.z() / 2);
            // Get the maximum of the size parameters
            max_size_ = std::max(outer_size_.x(), std::max(outer_size_.y(), outer_size_.z()));
        }

        /**
         * @brief Set the size of the box as an XYZ-vector
         * @param val outer_size_ of the box
         */
        void setSize(ROOT::Math::XYZVector val) { outer_size_ = std::move(val); }
        /**
         * @brief Set the size of the filling volume inside the box as an XYZ-vector
         * @param val filling_size_ of the box
         */
        void setFillingSize(ROOT::Math::XYZVector val) { inner_size_ = std::move(val); }

        // Set the override functions of PassiveMaterialModel
        G4SubtractionSolid* getSolid() override { return solid_; }
        G4Box* getFillingSolid() override { return filling_solid_; }
        double getMaxSize() override { return max_size_; }

    private:
        Configuration& config_;

        // G4VSolid returnables
        G4SubtractionSolid* solid_;
        G4Box* filling_solid_;
        double max_size_;

        // G4VSolid specifications
        ROOT::Math::XYZVector outer_size_;
        ROOT::Math::XYZVector inner_size_;
    };
} // namespace allpix

#endif /* ALLPIX_PASSIVE_MATERIAL_BOX_H */
