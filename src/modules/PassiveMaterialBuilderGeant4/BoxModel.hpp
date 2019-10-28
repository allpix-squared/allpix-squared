/**
 * @file
 * @brief Parameters of a hybrid pixel detector model
 *
 * @copyright Copyright (c) 2017-2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_PASSIVE_MATERIAL_BOX_H
#define ALLPIX_PASSIVE_MATERIAL_BOX_H

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
    class BoxModel : public PassiveMaterialModel {
    public:
        /**
         * @brief Constructs the hybrid pixel detector model
         * @param type Name of the model type
         * @param reader Configuration reader with description of the model
         */
        explicit BoxModel(Configuration& config)
            : PassiveMaterialModel(config), config_(config) {

            // Excess around the chip from the pixel grid
            setSize(config.get<ROOT::Math::XYZVector>("size"));
            setFillingSize(config.get<ROOT::Math::XYZVector>("filling_size", ROOT::Math::XYZVector()));
            std::string name = config_.getName();
            if(inner_size_.x() >= outer_size_.x()||inner_size_.y() >= outer_size_.y()||inner_size_.z() >= outer_size_.z()){ 
                throw InvalidValueError(config_, "inner_size", "inner_size cannot be larger than the outer_size");
            }
            auto outer_volume = new G4Box(name + "outer_volume", outer_size_.x() / 2, outer_size_.y() / 2, outer_size_.z() / 2);
            auto inner_volume = new G4Box(name + "inner_volume", inner_size_.x() / 2, inner_size_.y() / 2, inner_size_.z() / 2);
   
            solid_ = new G4SubtractionSolid(name + "_volume", outer_volume, inner_volume);
            filling_solid_ = new G4Box(name + "filling_volume", inner_size_.x() / 2, inner_size_.y() / 2, inner_size_.z() / 2);
            max_size_ = std::max(outer_size_.x(), std::max(outer_size_.y(), outer_size_.z()));
        }

        G4SubtractionSolid* GetSolid() override {return solid_;}
        G4Box* GetFillingSolid() override {return filling_solid_;}
        double GetMaxSize() override {return max_size_;}
        /**
         * @brief Set the XY-offset of the bumps from the center
         * @param val Offset from the pixel grid center
         */
        void setSize(ROOT::Math::XYZVector val) { outer_size_ = std::move(val); }
        /**
         * @brief Set the XY-offset of the bumps from the center
         * @param val Offset from the pixel grid center
         */
        void setFillingSize(ROOT::Math::XYZVector val) { inner_size_ = std::move(val); }


    private:
        G4SubtractionSolid* solid_;
        G4Box* filling_solid_;
        double max_size_;

        std::string type_;
        Configuration& config_;

        ROOT::Math::XYZVector outer_size_;
        ROOT::Math::XYZVector inner_size_;

    };
} // namespace allpix

#endif /* ALLPIX_PASSIVE_MATERIAL_BOX_H */
