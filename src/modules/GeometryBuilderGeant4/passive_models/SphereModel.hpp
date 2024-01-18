/**
 * @file
 * @brief Parameters of a sphere passive material model
 *
 * @copyright Copyright (c) 2019-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_PASSIVE_MATERIAL_SPHERE_H
#define ALLPIX_PASSIVE_MATERIAL_SPHERE_H

#include <string>

#include <G4Sphere.hh>

#include "PassiveMaterialModel.hpp"
#include "tools/geant4/geant4.h"

namespace allpix {

    /**
     * @ingroup PassiveMaterialModel
     * @brief Model of an sphere with inner and outer radius.
     */
    class SphereModel : public PassiveMaterialModel {
    public:
        /**
         * @brief Constructs the sphere passive material model
         * @param config Configuration with description of the model
         * @param geo_manager Pointer to the global geometry manager
         */
        explicit SphereModel(const Configuration& config, GeometryManager* geo_manager)
            : PassiveMaterialModel(config, geo_manager) {

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
            setStartingAnglePhi(config_.get<double>("starting_angle_phi", 0));
            setArcLengthPhi(config_.get<double>("arc_length_phi", 360 * CLHEP::deg));
            setStartingAngleTheta(config_.get<double>("starting_angle_theta", 0));
            setArcLengthTheta(config_.get<double>("arc_length_theta", 180 * CLHEP::deg));
            std::string name = config_.getName();

            // Limit the values that can be given
            if(inner_radius_ >= outer_radius_) {
                throw InvalidValueError(config_, "inner_radius", "inner_radius cannot be larger than the outer_radius");
            } else if(thickness > outer_radius_) {
                throw InvalidValueError(config_, "thickness", "thickness cannot be larger than the outer_radius");
            } else if(arc_length_phi_ > 360 * CLHEP::deg) {
                throw InvalidValueError(
                    config_, "arc_length_phi", "arc_length_phi exceeds the maximum value of 360 degrees");
            } else if(starting_angle_theta_ > 180 * CLHEP::deg) {
                throw InvalidValueError(
                    config_, "starting_angle_theta", "starting_angle_theta exceeds the maximum value of 180 degrees");
            } else if(starting_angle_theta_ + arc_length_theta_ > 180 * CLHEP::deg) {
                LOG(WARNING) << "starting_angle_theta and arc_length_theta combined cannot be larger than 180 degrees for '"
                             << name << "'. arc_length_theta will be set to 180deg - starting_angle_theta = "
                             << Units::display(180 * CLHEP::deg - starting_angle_theta_, "deg");
                setArcLengthTheta(180 * CLHEP::deg - starting_angle_theta_);
            }
            // Create the G4VSolids which make the sphere
            solid_ = make_shared_no_delete<G4Sphere>(name + "_volume",
                                                     inner_radius_,
                                                     outer_radius_,
                                                     starting_angle_phi_,
                                                     arc_length_phi_,
                                                     starting_angle_theta_,
                                                     arc_length_theta_);

            // Get the maximum of the size parameters
            max_size_ = 2 * outer_radius_;

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
        double inner_radius_;
        double outer_radius_;
        double starting_angle_phi_;
        double arc_length_phi_;
        double starting_angle_theta_;
        double arc_length_theta_;

        /**
         * @brief Set the inner radius of the sphere
         * @param val Inner radius of the sphere
         */
        void setInnerRadius(double val) { inner_radius_ = val; }
        /**
         * @brief Set the outer radius of the sphere
         * @param val Outer radius of the sphere
         */
        void setOuterRadius(double val) { outer_radius_ = val; }
        /**
         * @brief Set starting angle for the  azimuthal angle of the sphere in degrees
         * @param val Starting angle phi of the sphere
         */
        void setStartingAnglePhi(double val) { starting_angle_phi_ = val; }
        /**
         * @brief Set arc length of the azimuthal circumference in degrees
         * @param val Arc length phi of the sphere
         */
        void setArcLengthPhi(double val) { arc_length_phi_ = val; }
        /**
         * @brief Set starting angle of the Polar Angle of the sphere degrees
         * @param val Starting angle theta of the sphere
         */
        void setStartingAngleTheta(double val) { starting_angle_theta_ = val; }
        /**
         * @brief Set arc length of the polar circumference in degrees
         * @param val Arc length theta of the sphere
         */
        void setArcLengthTheta(double val) { arc_length_theta_ = val; }
    };
} // namespace allpix

#endif /* ALLPIX_PASSIVE_MATERIAL_SPHERE_H */
