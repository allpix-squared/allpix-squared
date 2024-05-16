/**
 * @file
 * @brief Parameters of a CONE passive material model
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_PASSIVE_MATERIAL_CONE_H
#define ALLPIX_PASSIVE_MATERIAL_CONE_H

#include <string>

#include <G4Cons.hh>

#include "PassiveMaterialModel.hpp"
#include "tools/geant4/geant4.h"

namespace allpix {
    class ConeModel : public PassiveMaterialModel {
    public:
        /**
         * @brief Constructs the Cone passive material model
         * @param config Configuration with description of the model
         * @param geo_manager Pointer to the global geometry manager
         */
        explicit ConeModel(const Configuration& config, GeometryManager* geo_manager)
            : PassiveMaterialModel(config, geo_manager) {

            // Set the CONE specifications
            /*
            must-fill :
                outer_radius_begin : outer radius at the begin (negative z) of the cone
                outer_radius_end : outer radius at the end (positive z) of the cone
                inner_radius_begin : inner radius at the begin (negative z) of the cone
                inner_radius_end : inner radius at the end (positive z) of the cone
                starting_angle   : start-angle ( default 0)
                arc_length       : length-of the arc (360 deg default)
            */
            setOuterRadius(config_.get<double>("outer_radius_begin"), config_.get<double>("outer_radius_end"));
            setInnerRadius(config_.get<double>("inner_radius_begin"), config_.get<double>("inner_radius_end"));
            setLength(config_.get<double>("length"));
            setStartingAngle(config_.get<double>("starting_angle", 0));
            setArcLength(config_.get<double>("arc_length", 360 * CLHEP::deg));
            std::string name = config_.getName();
            // Limit the values that can be given
            if(inner_radius_begin >= outer_radius_begin)
                throw InvalidValueError(
                    config_, "inner_radius_begin", "inner radius cannot be larger than the outer radius");
            if(inner_radius_end >= outer_radius_end)
                throw InvalidValueError(config_, "inner_radius_end", "inner radius cannot be larger than the outer radius");
            if(arc_length_ > 360 * CLHEP::deg) {
                throw InvalidValueError(config_, "arc_length", "arc_length exceeds the maximum value of 360 degrees");
            }

            solid_ = make_shared_no_delete<G4Cons>(name + "_volume",
                                                   inner_radius_begin,
                                                   outer_radius_begin,
                                                   inner_radius_end,
                                                   outer_radius_end,
                                                   length_ / 2.,
                                                   starting_angle_,
                                                   arc_length_);

            // Get the maximum of the size parameters
            max_size_ = std::max(2 * inner_radius_end, length_);

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
        double inner_radius_end;
        double outer_radius_end;
        double inner_radius_begin;
        double outer_radius_begin;

        double length_;
        double starting_angle_;
        double arc_length_;

        /**
         * @brief Set the inner radius of the CONE in the XY-plane at -halfLenght and +halfLength
         * @param val Inner radius of the CONE
         */
        void setInnerRadius(double val_begin, double val_end) {
            inner_radius_begin = val_begin;
            inner_radius_end = val_end;
        }
        /**
         * @brief Set the outer radius of the CONE in the XY-plane at -halfLenght and +halfLength
         * @param val Outer radius of the CONE
         */
        void setOuterRadius(double val_begin, double val_end) {
            outer_radius_begin = val_begin;
            outer_radius_end = val_end;
        }
        /**
         * @brief Set the length of the CONE in the Z-directior
         * @param val Offset from the pixel grid center
         */
        void setLength(double val) { length_ = val; }
        /**
         * @brief Set starting angle of the circumference of the CONE in degrees
         * @param val Starting angle of the CONE
         */
        void setStartingAngle(double val) { starting_angle_ = val; }
        /**
         * @brief Set arc length of the circumference in degrees
         * @param val Arc length of the CONE
         */
        void setArcLength(double val) { arc_length_ = val; }
    };
} // namespace allpix

#endif /* ALLPIX_PASSIVE_MATERIAL_CONE_H */
