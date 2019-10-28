/**
 * @file
 * @brief Parameters of a sphere passive material model
 *
 * @copyright Copyright (c) 2017-2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_PASSIVE_MATERIAL_SPHERE_H
#define ALLPIX_PASSIVE_MATERIAL_SPHERE_H

#include <string>
#include <utility>

#include <Math/Cartesian2D.h>
#include <Math/DisplacementVector2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

#include <G4Sphere.hh>

#include "PassiveMaterialModel.hpp"

namespace allpix {

    /**
     * @ingroup PassiveMaterialModel
     * @brief Model of an sphere with inner and outer radius. The sphere can be filled with a filling material.
     */
    class SphereModel : public PassiveMaterialModel {
    public:
        /**
         * @brief Constructs the sphere passive material model
         * @param config Configuration with description of the model
         */
        explicit SphereModel(Configuration& config)
            : PassiveMaterialModel(config), config_(config) {

            // Set the cylinder specifications
            setInnerRadius(config_.get<double>("inner_radius", 0));
            setOuterRadius(config_.get<double>("outer_radius"));
            setStartingAnglePhi(config_.get<double>("starting_angle_phi", 0));
            setArcLengthPhi(config_.get<double>("arc_length_phi", 360*CLHEP::deg));
            setStartingAngleTheta(config_.get<double>("starting_angle_theta", 0));
            setArcLengthTheta(config_.get<double>("arc_length_theta", 180*CLHEP::deg));
            std::string name = config_.getName();
            
            //Limit the values that can be given
             if(inner_radius >= outer_radius) {
                throw InvalidValueError(config_, "inner_radius", "inner_radius cannot be larger than the outer_radius");
            }
            if(arc_length_phi > 360*CLHEP::deg) {
                throw InvalidValueError(config_, "arc_length_phi", "arc_length_phi exceeds the maximum value of 360 degrees");
            }
            if(arc_length_theta > 180*CLHEP::deg) {
                throw InvalidValueError(config_, "arc_length_theta", "arc_length_theta exceeds the maximum value of 180 degrees");
            }
            // Create the G4VSolids which make the sphere
            solid_ = new G4Sphere(name + "_volume",
                                                            inner_radius,
                                                            outer_radius,
                                                            starting_angle_phi,
                                                            arc_length_phi,
                                                            starting_angle_theta,
                                                            arc_length_theta);

            filling_solid_ = new G4Sphere(name + "_filling_volume",
                                                                    0,
                                                                    inner_radius,
                                                                    starting_angle_phi,
                                                                    arc_length_phi,
                                                                    starting_angle_theta,
                                                                    arc_length_theta);
            //Get the maximum of the size parameters
            max_size_ = 2*outer_radius;
        }

        /**
         * @brief Set the inner radius of the sphere
         * @param val Inner radius of the sphere
         */
        void setInnerRadius(double val) { inner_radius = std::move(val); }
        /**
         * @brief Set the outer radius of the sphere
         * @param val Outer radius of the sphere
         */
        void setOuterRadius(double val) { outer_radius = std::move(val); }
        /**
         * @brief Set starting angle for the  azimuthal angle of the sphere in degrees
         * @param val Starting angle phi of the sphere
         */
        void setStartingAnglePhi(double val) { starting_angle_phi = std::move(val); }
        /**
         * @brief Set arc length of the azimuthal circumference in degrees
         * @param val Arc length phi of the sphere
         */
        void setArcLengthPhi(double val) { arc_length_phi = std::move(val); }
        /**
         * @brief Set starting angle of the Polar Angle of the sphere degrees
         * @param val Starting angle theta of the sphere
         */
        void setStartingAngleTheta(double val) { starting_angle_theta = std::move(val); }
        /**
         * @brief Set arc length of the polar circumference in degrees
         * @param val Arc length theta of the sphere
         */
        void setArcLengthTheta(double val) { arc_length_theta = std::move(val); }

        // Set the override functions of PassiveMaterialModel
        G4Sphere* GetSolid() override {return solid_;}
        G4Sphere* GetFillingSolid() override {return filling_solid_;}
        double GetMaxSize() override {return max_size_;}


    private:
        Configuration& config_;

        // G4VSolid returnables
        G4Sphere* solid_;
        G4Sphere* filling_solid_;
        double max_size_;

        // G4VSolid specifications
        double inner_radius;
        double outer_radius;
        double starting_angle_phi;
        double arc_length_phi;
        double starting_angle_theta;
        double arc_length_theta;

    };
} // namespace allpix

#endif /* ALLPIX_PASSIVE_MATERIAL_SPHERE_H */
