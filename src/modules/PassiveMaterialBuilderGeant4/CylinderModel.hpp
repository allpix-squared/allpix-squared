/**
 * @file
 * @brief Parameters of a hybrid pixel detector model
 *
 * @copyright Copyright (c) 2017-2019 CERN and the Allpix Squared authors.
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
     * @ingroup DetectorModels
     * @brief Model of a hybrid pixel detector. This a model where the sensor is bump-bonded to the chip
     */
    class CylinderModel : public PassiveMaterialModel {
    public:
        /**
         * @brief Constructs the hybrid pixel detector model
         * @param type Name of the model type
         * @param reader Configuration reader with description of the model
         */
        explicit CylinderModel(Configuration& config)
            : PassiveMaterialModel(config), config_(config) {

            // Excess around the chip from the pixel grid
            setInnerRadius(config_.get<double>("inner_radius", 0));
            setOuterRadius(config_.get<double>("outer_radius"));
            setLength(config_.get<double>("length"));
            setStartingAngle(config_.get<double>("starting_angle", 0));
            setArcLength(config_.get<double>("arc_length", 360*CLHEP::deg));

            std::string name = config_.getName();
            if(inner_radius >= outer_radius) {
                throw InvalidValueError(config_, "inner_radius", "inner_radius cannot be larger than the outer_raidus");
            }
            if(arc_length > 360*CLHEP::deg) {
                throw InvalidValueError(config_, "arc_length", "arc_length exceeds the maximum value of 360 degrees");
            }
            solid_ = new G4Tubs(name + "_volume",
                                                            inner_radius,
                                                            outer_radius,
                                                            length / 2,
                                                            starting_angle,
                                                            arc_length);
   
            filling_solid_ = new G4Tubs(name + "_filling_volume",
                                                                    0,
                                                                    inner_radius,
                                                                    length/2,
                                                                    starting_angle ,
                                                                    arc_length);
            max_size_ = std::max(2*outer_radius, length);
        
        
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
        void setLength(double val) { length = std::move(val); }
        /**
         * @brief Set the XY-offset of the bumps from the center
         * @param val Offset from the pixel grid center
         */
        void setStartingAngle(double val) { starting_angle = std::move(val); }
        /**
         * @brief Set the XY-offset of the bumps from the center
         * @param val Offset from the pixel grid center
         */
        void setArcLength(double val) { arc_length = std::move(val); }

        G4Tubs* GetSolid() override {return solid_;}
        G4Tubs* GetFillingSolid() override {return filling_solid_;}
        double GetMaxSize() override {return max_size_;}


    private:
        G4Tubs* solid_;
        G4Tubs* filling_solid_;
        double max_size_;

        std::string type_;
        Configuration& config_;

        double inner_radius;
        double outer_radius;
        double length;
        double starting_angle;
        double arc_length;

    };
} // namespace allpix

#endif /* ALLPIX_PASSIVE_MATERIAL_CYLINDER_H */
