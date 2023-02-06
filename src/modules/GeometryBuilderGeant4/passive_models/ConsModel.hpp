/**
 * @file
 * @brief Parameters of a CONS passive material model
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_PASSIVE_MATERIAL_CONS_H
#define ALLPIX_PASSIVE_MATERIAL_CONS_H

#include <string>

#include <G4Cons.hh>

#include "PassiveMaterialModel.hpp"
#include "tools/geant4/geant4.h"

namespace allpix {
    class ConsModel : public PassiveMaterialModel {
    public:
        /**
         * @brief Constructs the Cons passive material model
         * @param config Configuration with description of the model
         * @param geo_manager Pointer to the global geometry manager
         */        
        explicit ConsModel(const Configuration& config, GeometryManager* geo_manager)
            : PassiveMaterialModel(config, geo_manager) {

            // Set the CONS specifications
            /*
            must-fill : 
                outer_radius_mDz : at - half-length the outer radius
                outer_radius_pDz : at + half-length the outer radius
                inner_radius_mDz : at - half-length the inner radius (must be smaller than outer_radius_mDz)
                inner_radius_pDz : at + half-length the inner radius (must be smaller than outer_radius_mDz)
                starting_angle   : start-angle ( default 0)
                arc_length       : length-of the arc (360 deg default)
            */
            setOuterRadius(  config_.get<double>("outer_radius_mDz"), config_.get<double>("outer_radius_pDz"));
            setInnerRadius(  config_.get<double>("inner_radius_mDz"), config_.get<double>("inner_radius_pDz"));            
            setLength(       config_.get<double>("length"));
            setStartingAngle(config_.get<double>("starting_angle", 0));
            setArcLength(    config_.get<double>("arc_length", 360 * CLHEP::deg));
            std::string name = config_.getName();
            // Limit the values that can be given
            if(inner_radius_mDz >= outer_radius_mDz) throw InvalidValueError(config_, "inner_radius_mDz", "inner_radius (-half Legth)  cannot be larger than the outer_radius (+ half Length)");            
            if(inner_radius_pDz >= outer_radius_pDz) throw InvalidValueError(config_, "inner_radius_pDz", "inner_radius (+half Length) cannot be larger than the outer_raidus (+ half Length)");                     
            if(arc_length_ > 360 * CLHEP::deg) {
                throw InvalidValueError(config_, "arc_length", "arc_length exceeds the maximum value of 360 degrees");
            }

            solid_ = make_shared_no_delete<G4Cons>(
                name + "_volume", inner_radius_mDz, outer_radius_mDz, 
                                  inner_radius_pDz, outer_radius_pDz, 
                                  length_ / 2., 
                                  starting_angle_, arc_length_);

            // Get the maximum of the size parameters
            max_size_ = std::max(2 * inner_radius_pDz, length_);

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
        double inner_radius_pDz;
        double outer_radius_pDz;
        double inner_radius_mDz;
        double outer_radius_mDz;        

        double length_;
        double starting_angle_;
        double arc_length_;

        /**
         * @brief Set the inner radius of the CONS in the XY-plane at -halfLenght and +halfLength
         * @param val Inner radius of the CONS
         */
        void setInnerRadius(double val_mDz, double val_pDz ) { inner_radius_mDz = val_mDz; inner_radius_pDz = val_pDz; }
        /**
         * @brief Set the outer radius of the CONS in the XY-plane at -halfLenght and +halfLength
         * @param val Outer radius of the CONS
         */
        void setOuterRadius(double val_mDz, double val_pDz ) { outer_radius_mDz = val_mDz; outer_radius_pDz = val_pDz; }
        /**
         * @brief Set the length of the CONS in the Z-directior
         * @param val Offset from the pixel grid center
         */
        void setLength(double val) { length_ = val; }
        /**
         * @brief Set starting angle of the circumference of the CONS in degrees
         * @param val Starting angle of the CONS
         */
        void setStartingAngle(double val) { starting_angle_ = val; }
        /**
         * @brief Set arc length of the circumference in degrees
         * @param val Arc length of the CONS
         */
        void setArcLength(double val) { arc_length_ = val; }
    };
} // namespace allpix

#endif /* ALLPIX_PASSIVE_MATERIAL_CONS_H */
