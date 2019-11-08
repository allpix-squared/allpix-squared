/**
 * @file
 * @brief Parameters of a tube passive material model
 *
 * @copyright Copyright (c) 2017-2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_PASSIVE_MATERIAL_TUBE_H
#define ALLPIX_PASSIVE_MATERIAL_TUBE_H

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
     * @brief Model of a rectangular tube.
     */
    class TubeModel : public PassiveMaterialModel {
    public:
        /**
         * @brief Constructs the tube passive material model
         * @param config Configuration with description of the model
         */
        explicit TubeModel(Configuration& config) : PassiveMaterialModel(config), config_(config) {

            // Set the box specifications
            setOuterSize(config.get<ROOT::Math::XYZVector>("outer_size"));
            setInnerSize(config.get<ROOT::Math::XYVector>("inner_size", ROOT::Math::XYVector()));
            auto thickness = config.get<double>("thickness", 0);
            if(thickness != 0) {
                if(inner_size_ != ROOT::Math::XYVector()) {
                    throw InvalidValueError(config_, "thickness", "cannot have both 'thickness' and 'inner_size'");
                }

                setInnerSize({outer_size_.x() - thickness, outer_size_.y() - thickness});
            }

            std::string name = config_.getName();
            // Limit the values that can be given
            if(inner_size_.x() >= outer_size_.x() || inner_size_.y() >= outer_size_.y()) {
                throw InvalidValueError(config_, "inner_size_", "inner_size_ cannot be larger than the outer_size_");
            }

            // Create the G4VSolids which make the tube
            auto outer_volume =
                new G4Box(name + "_outer_volume", outer_size_.x() / 2, outer_size_.y() / 2, outer_size_.z() / 2);

            auto inner_volume =
                new G4Box(name + "_inner_volume", inner_size_.x() / 2, inner_size_.y() / 2, 1.1 * outer_size_.z() / 2);

            solid_ = new G4SubtractionSolid(name + "_volume", outer_volume, inner_volume);

            // Get the maximum of the size parameters
            max_size_ = std::max(outer_size_.x(), std::max(outer_size_.y(), outer_size_.z()));
        }
        /**
         * @brief Set the XYZ-value of the outer size of the tube
         * @param val Outer size of the tube
         */
        void setOuterSize(ROOT::Math::XYZVector val) { outer_size_ = std::move(val); }
        /**
         * @brief Set the XYZ-value of the outer size of the tube
         * @param val Outer size of the tube
         */
        void setInnerSize(ROOT::Math::XYVector val) { inner_size_ = std::move(val); }

        // Set the override functions of PassiveMaterialModel
        G4SubtractionSolid* getSolid() override { return solid_; }
        double getMaxSize() override { return max_size_; }

    private:
        Configuration& config_;

        // G4VSolid returnables
        G4SubtractionSolid* solid_;
        double max_size_;

        // G4VSolid specifications
        ROOT::Math::XYZVector outer_size_;
        ROOT::Math::XYVector inner_size_;
    };
} // namespace allpix

#endif /* ALLPIX_PASSIVE_MATERIAL_TUBE_H */
