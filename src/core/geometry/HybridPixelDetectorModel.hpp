/**
 * @file
 * @brief Parameters of a hybrid pixel detector model
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_HYBRID_PIXEL_DETECTOR_H
#define ALLPIX_HYBRID_PIXEL_DETECTOR_H

#include <string>
#include <utility>

#include <Math/Cartesian2D.h>
#include <Math/DisplacementVector2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

#include "PixelDetectorModel.hpp"

namespace allpix {

    /**
     * @ingroup DetectorModels
     * @brief Model of a hybrid pixel detector. This a model where the sensor is bump-bonded to the chip
     */
    class HybridPixelDetectorModel : public PixelDetectorModel {
    public:
        /**
         * @brief Constructs the hybrid pixel detector model
         * @param type Name of the model type
         * @param reader Configuration reader with description of the model
         */
        explicit HybridPixelDetectorModel(std::string type, std::shared_ptr<Chip> chip, const ConfigReader& reader)
            : PixelDetectorModel(std::move(type), chip, reader) {
            auto config = reader.getHeaderConfiguration();

            // Excess around the chip from the pixel grid
            auto default_chip_excess = config.get<double>("chip_excess", 0);
            setChipExcessTop(config.get<double>("chip_excess_top", default_chip_excess));
            setChipExcessBottom(config.get<double>("chip_excess_bottom", default_chip_excess));
            setChipExcessLeft(config.get<double>("chip_excess_left", default_chip_excess));
            setChipExcessRight(config.get<double>("chip_excess_right", default_chip_excess));

            // Set bump parameters
            setBumpCylinderRadius(config.get<double>("bump_cylinder_radius"));
            setBumpHeight(config.get<double>("bump_height"));
            setBumpSphereRadius(config.get<double>("bump_sphere_radius", 0));

            auto pitch = config.get<ROOT::Math::XYVector>("pixel_size");
            auto bump_offset = config.get<ROOT::Math::XYVector>("bump_offset", {0, 0});
            if(std::fabs(bump_offset.x()) > pitch.x() / 2.0 || std::fabs(bump_offset.y()) > pitch.y() / 2.0) {
                throw InvalidValueError(config, "bump_offset", "bump bond offset cannot be larger than half pixel pitch");
            }
            setBumpOffset(bump_offset);
        }

        /**
         * @brief Set the excess at the top of the chip (positive y-coordinate)
         * @param val Chip top excess
         */
        void setChipExcessTop(double val) { chip_excess_.at(0) = val; }
        /**
         * @brief Set the excess at the right of the chip (positive x-coordinate)
         * @param val Chip right excess
         */
        void setChipExcessRight(double val) { chip_excess_.at(1) = val; }
        /**
         * @brief Set the excess at the bottom of the chip (negative y-coordinate)
         * @param val Chip bottom excess
         */
        void setChipExcessBottom(double val) { chip_excess_.at(2) = val; }
        /**
         * @brief Set the excess at the left of the chip (negative x-coordinate)
         * @param val Chip left excess
         */
        void setChipExcessLeft(double val) { chip_excess_.at(3) = val; }

        /**
         * @brief extend the total size for the detector wrapper by potential shifts of the bump bond grid
         * @return Size of the hybrid pixel detector model
         */
        ROOT::Math::XYZVector getSize() const override {
            auto size = DetectorModel::getSize();
            auto bump_grid =
                getSensorSize() + 2 * ROOT::Math::XYZVector(std::fabs(bump_offset_.x()), std::fabs(bump_offset_.y()), 0);

            // Extend size unless it's already large enough to cover shifted bump bond grid:
            return ROOT::Math::XYZVector(
                std::max(size.x(), bump_grid.x()), std::max(size.y(), bump_grid.y()), std::max(size.z(), bump_grid.z()));
        }

    private:
        std::array<double, 4> chip_excess_{};

        double bump_sphere_radius_{};
        double bump_height_{};
        ROOT::Math::XYVector bump_offset_;
        double bump_cylinder_radius_{};
    };
} // namespace allpix

#endif /* ALLPIX_HYBRID_PIXEL_DETECTOR_H */
