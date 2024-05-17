/**
 * @file
 * @brief Parameters of a cone passive material model
 *
 * @copyright Copyright (c) 2023-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
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

            outer_radius_begin_ = config_.get<double>("outer_radius_begin");
            inner_radius_begin_ = config_.get<double>("inner_radius_begin", 0);
            outer_radius_end_ = config_.get<double>("outer_radius_end");
            inner_radius_end_ = config_.get<double>("inner_radius_end", 0);

            length_ = config_.get<double>("length");
            starting_angle_ = config_.get<double>("starting_angle", 0);
            arc_length_ = config_.get<double>("arc_length", 360 * CLHEP::deg);

            std::string name = config_.getName();

            // Limit the values that can be given
            if(inner_radius_begin_ >= outer_radius_begin_) {
                throw InvalidValueError(
                    config_, "inner_radius_begin", "inner radius cannot be larger than the outer radius");
            }
            if(inner_radius_end_ >= outer_radius_end_) {
                throw InvalidValueError(config_, "inner_radius_end", "inner radius cannot be larger than the outer radius");
            }
            if(arc_length_ > 360 * CLHEP::deg) {
                throw InvalidValueError(config_, "arc_length", "arc_length exceeds the maximum value of 360 degrees");
            }

            solid_ = make_shared_no_delete<G4Cons>(name + "_volume",
                                                   inner_radius_begin_,
                                                   outer_radius_begin_,
                                                   inner_radius_end_,
                                                   outer_radius_end_,
                                                   length_ / 2.,
                                                   starting_angle_,
                                                   arc_length_);

            // Get the maximum of the size parameters
            max_size_ = std::max(2 * outer_radius_begin_, 2 * outer_radius_end_);
            max_size_ = std::max(max_size_, length_);

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
        double inner_radius_end_;
        double outer_radius_end_;
        double inner_radius_begin_;
        double outer_radius_begin_;

        double length_;
        double starting_angle_;
        double arc_length_;
    };
} // namespace allpix

#endif /* ALLPIX_PASSIVE_MATERIAL_CONE_H */
