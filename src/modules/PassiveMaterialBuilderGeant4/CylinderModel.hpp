/**
 * @file
 * @brief Parameters of a cylinder passive material model
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
     * @ingroup PassiveMaterialModel
     * @brief Model of an cylinder with inner and outer radius. The cylinder can be filled with a filling material.
     */
    class CylinderModel : public PassiveMaterialModel {
    public:
         /**
         * @brief Constructs the cylinder passive material model
         * @param config Configuration with description of the model
         */
        explicit CylinderModel(Configuration& config)
            : PassiveMaterialModel(config), config_(config) {

            // Set the cylinder specifications
            setInnerRadius(config_.get<double>("inner_radius", 0));
            setOuterRadius(config_.get<double>("outer_radius"));
            setLength(config_.get<double>("length"));
            setStartingAngle(config_.get<double>("starting_angle", 0));
            setArcLength(config_.get<double>("arc_length", 360*CLHEP::deg));
            std::string name = config_.getName();

            //Limit the values that can be given
            if(inner_radius >= outer_radius) {
                throw InvalidValueError(config_, "inner_radius", "inner_radius cannot be larger than the outer_raidus");
            }
            if(arc_length > 360*CLHEP::deg) {
                throw InvalidValueError(config_, "arc_length", "arc_length exceeds the maximum value of 360 degrees");
            }

            // Create the G4VSolids which make the cylinder
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
            //Get the maximum of the size parameters
            max_size_ = std::max(2*outer_radius, length);
        
        
        }

        /**
         * @brief Set the inner radius of the cylinder in the XY-plane
         * @param val Inner radius of the cylinder
         */
        void setInnerRadius(double val) { inner_radius = std::move(val); }
        /**
         * @brief Set the outer radius of the cylinder in the XY-plane
         * @param val Outer radius of the cylinder
         */
        void setOuterRadius(double val) { outer_radius = std::move(val); }
        /**
         * @brief Set the length of the cylinder in the Z-directior
         * @param val Offset from the pixel grid center
         */
        void setLength(double val) { length = std::move(val); }
        /**
         * @brief Set starting angle of the circumference of the cylinder in degrees
         * @param val Starting angle of the cylinder
         */
        void setStartingAngle(double val) { starting_angle = std::move(val); }
        /**
         * @brief Set arc length of the circumference in degrees
         * @param val Arc length of the cylinder
         */
        void setArcLength(double val) { arc_length = std::move(val); }

        // Set the override functions of PassiveMaterialModel
        G4Tubs* GetSolid() override {return solid_;}
        G4Tubs* GetFillingSolid() override {return filling_solid_;}
        double GetMaxSize() override {return max_size_;}


    private:
        Configuration& config_;

        // G4VSolid returnables
        G4Tubs* solid_;
        G4Tubs* filling_solid_;
        double max_size_;


        // G4VSolid specifications
        double inner_radius;
        double outer_radius;
        double length;
        double starting_angle;
        double arc_length;
    };
} // namespace allpix

#endif /* ALLPIX_PASSIVE_MATERIAL_CYLINDER_H */
