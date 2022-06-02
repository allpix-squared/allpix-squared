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
        explicit HybridPixelDetectorModel(std::string type, const ConfigReader& reader)
            : PixelDetectorModel(std::move(type), reader) {
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
         * @brief Get size of the chip
         * @return Size of the chip
         *
         * Calculated from \ref DetectorModel::getMatrixSize "pixel grid size", chip excess and chip thickness
         */
        ROOT::Math::XYZVector getChipSize() const override {
            ROOT::Math::XYZVector excess_thickness(
                (chip_excess_.at(1) + chip_excess_.at(3)), (chip_excess_.at(0) + chip_excess_.at(2)), chip_.getThickness());
            return getMatrixSize() + excess_thickness;
        }
        /**
         * @brief Get center of the chip in local coordinates
         * @return Center of the chip
         *
         * The center of the chip as given by \ref DetectorModel::getChipCenter() with extra offset for bump bonds.
         */
        ROOT::Math::XYZPoint getChipCenter() const override {
            ROOT::Math::XYZVector offset((chip_excess_.at(1) - chip_excess_.at(3)) / 2.0,
                                         (chip_excess_.at(0) - chip_excess_.at(2)) / 2.0,
                                         getSensorSize().z() / 2.0 + getChipSize().z() / 2.0 + getBumpHeight());
            return getMatrixCenter() + offset;
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

        /**
         * @brief Return all layers of support
         * @return List of all the support layers
         *
         * The center of the support is adjusted to take the bump bonds into account
         */
        std::vector<SupportLayer> getSupportLayers() const override {
            auto ret_layers = DetectorModel::getSupportLayers();

            for(auto& layer : ret_layers) {
                if(layer.location_ == "chip") {
                    layer.center_.SetZ(layer.center_.z() + getBumpHeight());
                }
            }

            return ret_layers;
        }

        /**
         * @brief Get the center of the bump bonds in local coordinates
         * @return Center of the bump bonds
         *
         * The bump bonds are aligned with the grid with an optional XY-offset. The z-offset is calculated with the sensor
         * and chip offsets taken into account.
         */
        virtual ROOT::Math::XYZPoint getBumpsCenter() const {
            ROOT::Math::XYZVector offset(
                bump_offset_.x(), bump_offset_.y(), getSensorSize().z() / 2.0 + getBumpHeight() / 2.0);
            return getMatrixCenter() + offset;
        }
        /**
         * @brief Get the radius of the sphere of every individual bump bond (union solid with cylinder)
         * @return Radius of bump bond sphere
         */
        double getBumpSphereRadius() const { return bump_sphere_radius_; }
        /**
         * @brief Set the radius of the sphere of every individual bump bond  (union solid with cylinder)
         * @param val Radius of bump bond sphere
         */
        void setBumpSphereRadius(double val) { bump_sphere_radius_ = val; }
        /**
         * @brief Get the radius of the cylinder of every individual bump bond  (union solid with sphere)
         * @return Radius of bump bond cylinder
         */
        double getBumpCylinderRadius() const { return bump_cylinder_radius_; }
        /**
         * @brief Set the radius of the cylinder of every individual bump bond  (union solid with sphere)
         * @param val Radius of bump bond cylinder
         */
        void setBumpCylinderRadius(double val) { bump_cylinder_radius_ = val; }
        /**
         * @brief Get the height of the bump bond cylinder, determining the offset between sensor and chip
         * @return Height of the bump bonds
         */
        double getBumpHeight() const { return bump_height_; }
        /**
         * @brief Set the height of the bump bond cylinder, determining the offset between sensor and chip
         * @param val Height of the bump bonds
         */
        void setBumpHeight(double val) { bump_height_ = val; }
        /**
         * @brief Set the XY-offset of the bumps from the center
         * @param val Offset from the pixel grid center
         */
        void setBumpOffset(ROOT::Math::XYVector val) { bump_offset_ = std::move(val); }

    private:
        std::array<double, 4> chip_excess_{};

        double bump_sphere_radius_{};
        double bump_height_{};
        ROOT::Math::XYVector bump_offset_;
        double bump_cylinder_radius_{};
    };
} // namespace allpix

#endif /* ALLPIX_HYBRID_PIXEL_DETECTOR_H */
