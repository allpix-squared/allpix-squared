/**
 * @file
 * @brief Parameters of a box passive material model
 *
 * @copyright Copyright (c) 2019-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_PASSIVE_MATERIAL_BOX_H
#define ALLPIX_PASSIVE_MATERIAL_BOX_H

#include <string>

#include <G4Box.hh>
#include <G4SubtractionSolid.hh>
#include <G4VSolid.hh>

#include "PassiveMaterialModel.hpp"
#include "tools/geant4/geant4.h"

namespace allpix {

    /**
     * @ingroup PassiveMaterialModel
     * @brief Model of a rectangular box.
     */
    class BoxModel : public PassiveMaterialModel {
    public:
        /**
         * @brief Constructs the box passive material model
         * @param config Configuration with description of the model
         * @param geo_manager Pointer to the global geometry manager
         */
        explicit BoxModel(const Configuration& config, GeometryManager* geo_manager)
            : PassiveMaterialModel(config, geo_manager) {

            // Set the box specifications
            setOuterSize(config_.get<ROOT::Math::XYZVector>("size"));
            setInnerSize(config_.get<ROOT::Math::XYZVector>("inner_size", ROOT::Math::XYZVector()));
            auto thickness = config_.get<double>("thickness", 0);
            if(thickness != 0) {
                if(inner_size_ != ROOT::Math::XYZVector()) {
                    throw InvalidValueError(config_, "thickness", "cannot have both 'thickness' and 'inner_size'");
                }

                setInnerSize({outer_size_.x() - thickness, outer_size_.y() - thickness, outer_size_.z() - thickness});
            }

            std::string name = config_.getName();
            // Limit the values that can be given
            if(inner_size_.x() > outer_size_.x() || inner_size_.y() > outer_size_.y() || inner_size_.z() > outer_size_.z()) {
                throw InvalidValueError(config_, "inner_size", "inner_size_ cannot be larger than the outer_size_");
            }

            // Add infinitesimal amount of material if the inner and outer size are equal to avoid artificial remnant
            // surfaces
            if(outer_size_.x() - inner_size_.x() < std::numeric_limits<double>::epsilon()) {
                setInnerSize(ROOT::Math::XYZVector(inner_size_.x() * 2, inner_size_.y(), inner_size_.z()));
            }
            if(outer_size_.y() - inner_size_.y() < std::numeric_limits<double>::epsilon()) {
                setInnerSize(ROOT::Math::XYZVector(inner_size_.x(), inner_size_.y() * 2, inner_size_.z()));
            }
            if(outer_size_.z() - inner_size_.z() < std::numeric_limits<double>::epsilon()) {
                setInnerSize(ROOT::Math::XYZVector(inner_size_.x(), inner_size_.y(), inner_size_.z() * 2));
            }

            // Create the G4VSolids which make the Box
            auto outer_volume = make_shared_no_delete<G4Box>(
                name + "_outer_volume", outer_size_.x() / 2, outer_size_.y() / 2, outer_size_.z() / 2);
            if(inner_size_ == ROOT::Math::XYZVector()) {
                solid_ = outer_volume;
            } else {
                auto inner_volume = make_shared_no_delete<G4Box>(
                    name + "_inner_volume", inner_size_.x() / 2, inner_size_.y() / 2, inner_size_.z() / 2);

                solid_ = std::make_shared<G4SubtractionSolid>(name + "_volume", outer_volume.get(), inner_volume.get());

                // Keep references to the solids created because G4SubtractionSolid does not assume ownership
                solids_.push_back(outer_volume);
                solids_.push_back(inner_volume);
            }
            // Get the maximum of the size parameters
            max_size_ = std::max(outer_size_.x(), std::max(outer_size_.y(), outer_size_.z()));

            LOG(DEBUG) << "Adding points for volume";
            add_points();
        }

        // Set the override functions of PassiveMaterialModel
        double getMaxSize() const override { return max_size_; }

    private:
        std::shared_ptr<G4VSolid> get_solid() const override { return solid_; }

        // G4VSolid returnables
        std::shared_ptr<G4VSolid> solid_;

        // G4VSolid specifications
        ROOT::Math::XYZVector outer_size_;
        ROOT::Math::XYZVector inner_size_;

        /**
         * @brief Set the XYZ-value of the outer size of the box
         * @param val Outer size of the box
         */
        void setOuterSize(ROOT::Math::XYZVector val) { outer_size_ = std::move(val); }
        /**
         * @brief Set the XYZ-value of the inner size of the box
         * @param val Inner size of the box
         */
        void setInnerSize(ROOT::Math::XYZVector val) { inner_size_ = std::move(val); }
    };
} // namespace allpix

#endif /* ALLPIX_PASSIVE_MATERIAL_BOX_H */
