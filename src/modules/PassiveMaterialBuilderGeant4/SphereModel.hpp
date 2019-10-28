/**
 * @file
 * @brief Parameters of a hybrid pixel detector model
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
     * @ingroup DetectorModels
     * @brief Model of a hybrid pixel detector. This a model where the sensor is bump-bonded to the chip
     */
    class SphereModel : public PassiveMaterialModel {
    public:
        /**
         * @brief Constructs the hybrid pixel detector model
         * @param type Name of the model type
         * @param reader Configuration reader with description of the model
         */
        explicit SphereModel(Configuration& config)
            : PassiveMaterialModel(config), config_(config) {

                // Excess around the chip from the pixel grid
                setInnerRadius(config_.get<double>("inner_radius", 0));
                setOuterRadius(config_.get<double>("outer_radius"));
                setStartingAnglePhi(config_.get<double>("starting_angle_phi", 0));
                setArcLengthPhi(config_.get<double>("arc_length_phi", 360*CLHEP::deg));
                setStartingAngleTheta(config_.get<double>("starting_angle_theta", 0));
                setArcLengthTheta(config_.get<double>("arc_length_theta", 180*CLHEP::deg));
                std::string name = config_.getName();
                 if(inner_radius >= outer_radius) {
                    throw InvalidValueError(config_, "inner_radius", "inner_radius cannot be larger than the outer_radius");
                }
                if(arc_length_phi > 360*CLHEP::deg) {
                    throw InvalidValueError(config_, "arc_length_phi", "arc_length_phi exceeds the maximum value of 360 degrees");
                }
                if(arc_length_theta > 180*CLHEP::deg) {
                    throw InvalidValueError(config_, "arc_length_theta", "arc_length_theta exceeds the maximum value of 180 degrees");
                }
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
                max_size_ = 2*outer_radius;
        }

        /**
         * @brief Set the XY-offset of the bumps from the center
         * @param val Offset from the pixel grid center
         */
        void setInnerRadius(double val) { inner_radius = std::move(val); }
        /**
         * @brief Set the XY-offset of the bumps from the center
         * @param val Offset from the pixel grid center
         */
        void setOuterRadius(double val) { outer_radius = std::move(val); }
        /**
         * @brief Set the XY-offset of the bumps from the center
         * @param val Offset from the pixel grid center
         */
        void setStartingAnglePhi(double val) { starting_angle_phi = std::move(val); }
        /**
         * @brief Set the XY-offset of the bumps from the center
         * @param val Offset from the pixel grid center
         */
        void setArcLengthPhi(double val) { arc_length_phi = std::move(val); }
        /**
         * @brief Set the XY-offset of the bumps from the center
         * @param val Offset from the pixel grid center
         */
        void setStartingAngleTheta(double val) { starting_angle_theta = std::move(val); }
        /**
         * @brief Set the XY-offset of the bumps from the center
         * @param val Offset from the pixel grid center
         */
        void setArcLengthTheta(double val) { arc_length_theta = std::move(val); }

        G4Sphere* GetSolid() override {return solid_;}
        G4Sphere* GetFillingSolid() override {return filling_solid_;}
        double GetMaxSize() override {return max_size_;}


    private:
        G4Sphere* solid_;
        G4Sphere* filling_solid_;
        double max_size_;

        std::string type_;
        Configuration& config_;

        double inner_radius;
        double outer_radius;
        double starting_angle_phi;
        double arc_length_phi;
        double starting_angle_theta;
        double arc_length_theta;

    };
} // namespace allpix

#endif /* ALLPIX_PASSIVE_MATERIAL_SPHERE_H */
