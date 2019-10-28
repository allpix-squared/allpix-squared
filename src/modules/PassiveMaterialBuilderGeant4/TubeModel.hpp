/**
 * @file
 * @brief Parameters of a hybrid pixel detector model
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
     * @ingroup DetectorModels
     * @brief Model of a hybrid pixel detector. This a model where the sensor is bump-bonded to the chip
     */
    class TubeModel : public PassiveMaterialModel {
    public:
        /**
         * @brief Constructs the hybrid pixel detector model
         * @param type Name of the model type
         * @param reader Configuration reader with description of the model
         */
        explicit TubeModel(Configuration& config)
            : PassiveMaterialModel(config), config_(config) {

            // Excess around the chip from the pixel grid
            setOuterWidth(config_.get<double>("outer_width"));
            setOuterHeight(config_.get<double>("outer_height"));
            setInnerWidth(config.get<double>("inner_width", 0));
            setInnerHeight(config.get<double>("inner_height", 0));
            setLength(config.get<double>("length"));
            std::string name = config_.getName();
            if(inner_width >= outer_width){ 
                throw InvalidValueError(config_, "inner_width", "inner_width cannot be larger than the outer_width");
            }
            if(inner_height >= outer_height) {
                throw InvalidValueError(config_, "inner_height", "inner_height cannot be larger than the outer_height");
            }
            auto outer_volume =
                new G4Box(name + "_outer_volume", outer_width / 2, outer_height / 2, length / 2);

            auto inner_volume = new G4Box(
                name + "_inner_volume", inner_width / 2, inner_height / 2, 1.1 * length / 2);

            solid_ =
                new G4SubtractionSolid(name + "_volume", outer_volume, inner_volume);

            filling_solid_ = new G4Box(
                name + "_filling_volume", inner_width / 2, inner_height / 2, length / 2); 

            max_size_ = std::max(outer_width, std::max(outer_height, length));
        }        
        /**
         * @brief Set the XY-offset of the bumps from the center
         * @param val Offset from the pixel grid center
         */
        void setOuterWidth(double val) { outer_width = std::move(val); }
        /**
         * @brief Set the XY-offset of the bumps from the center
         * @param val Offset from the pixel grid center
         */
        void setOuterHeight(double val) { outer_height = std::move(val); }
        /**
         * @brief Set the XY-offset of the bumps from the center
         * @param val Offset from the pixel grid center
         */
        void setInnerWidth(double val) { inner_width = std::move(val); }
        /**
         * @brief Set the XY-offset of the bumps from the center
         * @param val Offset from the pixel grid center
         */
        void setInnerHeight(double val) { inner_height = std::move(val); }
        /**
         * @brief Set the XY-offset of the bumps from the center
         * @param val Offset from the pixel grid center
         */
        void setLength(double val) { length = std::move(val); }

        G4SubtractionSolid* GetSolid() override {return solid_;}
        G4Box* GetFillingSolid() override {return filling_solid_;}
        double GetMaxSize() override {return max_size_;}


    private:
        G4SubtractionSolid* solid_;
        G4Box* filling_solid_;
        double max_size_;

        std::string type_;
        Configuration& config_;
        ROOT::Math::XYZVector box_outer_size_;
        ROOT::Math::XYZVector box_inner_size_;

        double outer_width;
        double outer_height;
        double inner_width;
        double inner_height;
        double length;

    };
} // namespace allpix

#endif /* ALLPIX_PASSIVE_MATERIAL_TUBE_H */
