/**
 * @file
 * @brief Parameters of a tube passive material model
 *
 * @copyright Copyright (c) 2017-2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_PASSIVE_MATERIAL_TUBE_H
#define ALLPIX_PASSIVE_MATERIAL_TUBE_H

#include <string>
#include <utility>

#include <Math/Cartesian2D.h>
#include <Math/DisplacementVector2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

#include <G4Box.hh>
#include <G4SubtractionSolid.hh>

#include "PassiveMaterialModel.hpp"

namespace allpix {

    /**
     * @ingroup PassiveMaterialModel
     * @brief Model of a rectangular tube. The tube can be filled with a filling material.
     */
    class TubeModel : public PassiveMaterialModel {
    public:
        /**
         * @brief Constructs the tube passive material model
         * @param config Configuration with description of the model
         */
        explicit TubeModel(Configuration& config) : PassiveMaterialModel(config), config_(config) {

            // Set the box specifications
            setOuterWidth(config_.get<double>("outer_width"));
            setOuterHeight(config_.get<double>("outer_height"));
            setInnerWidth(config.get<double>("inner_width", 0));
            setInnerHeight(config.get<double>("inner_height", 0));
            setLength(config.get<double>("length"));
            std::string name = config_.getName();
            // Limit the values that can be given
            if(inner_width >= outer_width) {
                throw InvalidValueError(config_, "inner_width", "inner_width cannot be larger than the outer_width");
            }
            if(inner_height >= outer_height) {
                throw InvalidValueError(config_, "inner_height", "inner_height cannot be larger than the outer_height");
            }
            // Create the G4VSolids which make the tube
            auto outer_volume = new G4Box(name + "_outer_volume", outer_width / 2, outer_height / 2, length / 2);

            auto inner_volume = new G4Box(name + "_inner_volume", inner_width / 2, inner_height / 2, 1.1 * length / 2);

            solid_ = new G4SubtractionSolid(name + "_volume", outer_volume, inner_volume);

            filling_solid_ = new G4Box(name + "_filling_volume", inner_width / 2, inner_height / 2, length / 2);

            // Get the maximum of the size parameters
            max_size_ = std::max(outer_width, std::max(outer_height, length));
        }
        /**
         * @brief Set the X-value of the outer size of the tube
         * @param val Outer width of the tube
         */
        void setOuterWidth(double val) { outer_width = std::move(val); }
        /**
         * @brief Set the Y-value of the outer size of the tube
         * @param val Outer height of the tube
         */
        void setOuterHeight(double val) { outer_height = std::move(val); }
        /**
         * @brief Set the X-value of the inner size of the tube
         * @param val Inner width of the tube
         */
        void setInnerWidth(double val) { inner_width = std::move(val); }
        /**
         * @brief Set the Y-value of the inner size of the tube
         * @param val Inner height of the tube
         */
        void setInnerHeight(double val) { inner_height = std::move(val); }
        /**
         * @brief Set the z-value of the outer size of the tube
         * @param val Length of the tube
         */
        void setLength(double val) { length = std::move(val); }

        // Set the override functions of PassiveMaterialModel
        G4SubtractionSolid* getSolid() override { return solid_; }
        G4Box* getFillingSolid() override { return filling_solid_; }
        double getMaxSize() override { return max_size_; }

    private:
        Configuration& config_;

        // G4VSolid returnables
        G4SubtractionSolid* solid_;
        G4Box* filling_solid_;
        double max_size_;

        // G4VSolid specifications
        double outer_width;
        double outer_height;
        double inner_width;
        double inner_height;
        double length;
    };
} // namespace allpix

#endif /* ALLPIX_PASSIVE_MATERIAL_TUBE_H */
