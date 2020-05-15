/**
 * @file
 * @brief Parameters of a cylinder passive material model
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_PASSIVE_MATERIAL_CYLINDER_H
#define ALLPIX_PASSIVE_MATERIAL_CYLINDER_H

#include <string>
#include <utility>

#include <Math/Cartesian2D.h>
#include <Math/DisplacementVector2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

#include <G4Tubs.hh>

#include "PassiveMaterialModel.hpp"

namespace allpix {

    /**
     * @ingroup PassiveMaterialModel
     * @brief Model of an cylinder with inner and outer radius.
     */
    class CylinderModel : public PassiveMaterialModel {
    public:
        /**
         * @brief Constructs the cylinder passive material model
         * @param config Configuration with description of the model
         */
        CylinderModel(Configuration config, GeometryManager* geo_manager);

        void calculate_size() {

            // Set the cylinder specifications
            setOuterRadius(config_.get<double>("outer_radius"));
            setInnerRadius(config_.get<double>("inner_radius", 0));
            auto thickness = config_.get<double>("thickness", 0);
            if(thickness != 0) {
                if(inner_radius_ != 0) {
                    throw InvalidValueError(config_, "thickness", "cannot have both 'thickness' and 'inner_radius'");
                }
                setInnerRadius(outer_radius_ - thickness);
            }
            setLength(config_.get<double>("length"));
            setStartingAngle(config_.get<double>("starting_angle", 0));
            setArcLength(config_.get<double>("arc_length", 360 * CLHEP::deg));
            std::string name = config_.getName();

            // Limit the values that can be given
            if(inner_radius_ >= outer_radius_) {
                throw InvalidValueError(config_, "inner_radius", "inner_radius cannot be larger than the outer_raidus");
            }
            if(arc_length_ > 360 * CLHEP::deg) {
                throw InvalidValueError(config_, "arc_length", "arc_length exceeds the maximum value of 360 degrees");
            }

            // Create the G4VSolids which make the cylinder
            solid_ = new G4Tubs(name + "_volume", inner_radius_, outer_radius_, length_ / 2, starting_angle_, arc_length_);

            // Get the maximum of the size parameters
            max_size_ = std::max(2 * outer_radius_, length_);
        }

        // Set the override functions of PassiveMaterialModel
        G4Tubs* getSolid() const override { return solid_; }
        double getMaxSize() const override { return max_size_; }

    private:
        // G4VSolid returnables
        G4Tubs* solid_;

        // G4VSolid specifications
        double inner_radius_;
        double outer_radius_;
        double length_;
        double starting_angle_;
        double arc_length_;

        /**
         * @brief Set the inner radius of the cylinder in the XY-plane
         * @param val Inner radius of the cylinder
         */
        void setInnerRadius(double val) { inner_radius_ = std::move(val); }
        /**
         * @brief Set the outer radius of the cylinder in the XY-plane
         * @param val Outer radius of the cylinder
         */
        void setOuterRadius(double val) { outer_radius_ = std::move(val); }
        /**
         * @brief Set the length of the cylinder in the Z-directior
         * @param val Offset from the pixel grid center
         */
        void setLength(double val) { length_ = std::move(val); }
        /**
         * @brief Set starting angle of the circumference of the cylinder in degrees
         * @param val Starting angle of the cylinder
         */
        void setStartingAngle(double val) { starting_angle_ = std::move(val); }
        /**
         * @brief Set arc length of the circumference in degrees
         * @param val Arc length of the cylinder
         */
        void setArcLength(double val) { arc_length_ = std::move(val); }
    };
} // namespace allpix

#endif /* ALLPIX_PASSIVE_MATERIAL_CYLINDER_H */
