/**
 * @file
 * @brief Parameters of a box passive material model
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
     * @ingroup PassiveMaterialModel
     * @brief Model of a rectangular box.
     */
    class BoxModel : public PassiveMaterialModel {
    public:
        /**
         * @brief Constructs the box passive material model
         * @param config Configuration with description of the model
         */
        explicit BoxModel(Configuration& config) : PassiveMaterialModel(config), config_(config) {

            // Set the box specifications
            setSize(config.get<ROOT::Math::XYZVector>("size"));
            std::string name = config_.getName();

            // Create the G4VSolids which make the box
            solid_ = new G4Box(name + "_volume", size_.x() / 2, size_.y() / 2, size_.z() / 2);

            // Get the maximum of the size parameters
            max_size_ = std::max(size_.x(), std::max(size_.y(), size_.z()));
        }

        /**
         * @brief Set the size of the box as an XYZ-vector
         * @param val outer_size_ of the box
         */
        void setSize(ROOT::Math::XYZVector val) { size_ = std::move(val); }

        // Set the override functions of PassiveMaterialModel
        G4Box* getSolid() override { return solid_; }
        double getMaxSize() override { return max_size_; }

    private:
        Configuration& config_;

        // G4VSolid returnables
        G4Box* solid_;
        double max_size_;

        // G4VSolid specifications
        ROOT::Math::XYZVector size_;
    };
} // namespace allpix

#endif /* ALLPIX_PASSIVE_MATERIAL_BOX_H */
