/**
 * @file
 * @brief Definition of detector assemblies
 *
 * @copyright Copyright (c) 2022-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_DETECTOR_ASSEMBLY_H
#define ALLPIX_DETECTOR_ASSEMBLY_H

#include <Math/Point2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

#include "core/config/ConfigReader.hpp"
#include "core/config/exceptions.h"
#include "tools/ROOT.h"

namespace allpix {
    /**
     * @brief Helper class to hold information on the detector assembly
     */
    class DetectorAssembly {
    public:
        /**
         * Detector assembly constructor
         * @param header_config Configuration reference holding the unnamed section fo detector configuration
         */
        explicit DetectorAssembly(Configuration& header_config) {

            // Chip thickness
            thickness_ = header_config.get<double>("chip_thickness", 0);
        }

        ///@{
        /**
         * @brief Deleted default constructor
         */
        DetectorAssembly() = delete;
        virtual ~DetectorAssembly() = default;
        ///@}

        /**
         * @brief Get the thickness of the chip
         * @return Thickness of the chip
         */
        double getChipThickness() const { return thickness_; }

        /**
         * @brief Get excess of the chip beyond the active matrix
         * @return Chip excess
         */
        ROOT::Math::XYVector getChipExcess() const {
            return {(excess_.at(1) + excess_.at(3)), (excess_.at(0) + excess_.at(2))};
        }

        /**
         * @brief Get the offset of the chip center with respect to the matrix center
         * @return Chip offset
         */
        virtual ROOT::Math::XYZVector getChipOffset() const {
            return {(excess_.at(1) - excess_.at(3)), (excess_.at(0) - excess_.at(2)), 0};
        }

    protected:
        std::array<double, 4> excess_{};

    private:
        double thickness_{};
    };

    /**
     * A hybrid detector assembly describing a setup with separate sensor and readout ASIC
     */
    class HybridAssembly : public DetectorAssembly {
    public:
        /**
         * Constructor for hybrid assemblies
         * @param header_config  Configuration reference holding the unnamed section fo detector configuration
         */
        explicit HybridAssembly(Configuration& header_config) : DetectorAssembly(header_config) {

            // Excess around the chip from the pixel grid
            auto default_assembly_excess = header_config.get<double>("chip_excess", 0);
            excess_.at(0) = header_config.get<double>("chip_excess_top", default_assembly_excess);
            excess_.at(1) = header_config.get<double>("chip_excess_right", default_assembly_excess);
            excess_.at(2) = header_config.get<double>("chip_excess_bottom", default_assembly_excess);
            excess_.at(3) = header_config.get<double>("chip_excess_left", default_assembly_excess);

            // Set bump parameters
            bump_cylinder_radius_ = header_config.get<double>("bump_cylinder_radius");
            bump_height_ = header_config.get<double>("bump_height");
            bump_sphere_radius_ = header_config.get<double>("bump_sphere_radius", 0);

            auto pitch = header_config.get<ROOT::Math::XYVector>("pixel_size");
            bump_offset_ = header_config.get<ROOT::Math::XYVector>("bump_offset", {0, 0});
            if(std::fabs(bump_offset_.x()) > pitch.x() / 2.0 || std::fabs(bump_offset_.y()) > pitch.y() / 2.0) {
                throw InvalidValueError(
                    header_config, "bump_offset", "bump bond offset cannot be larger than half pixel pitch");
            }
        }

        /**
         * @brief Get the offset of the chip center with respect to the matrix center, taking into account the additional
         * offset caused by the layer of bump bonds
         * @return Chip offset
         */
        ROOT::Math::XYZVector getChipOffset() const override {
            return {(excess_.at(1) - excess_.at(3)), (excess_.at(0) - excess_.at(2)), bump_height_};
        }

        /**
         * @brief Get the center of the bump bonds in local coordinates
         * @return Center of the bump bonds
         *
         * The bump bonds are aligned with the grid with an optional XY-offset. The z-offset is calculated with the
         * sensor and chip offsets taken into account.
         */
        ROOT::Math::XYZVector getBumpsOffset() const { return {bump_offset_.x(), bump_offset_.y(), bump_height_ / 2.0}; }
        /**
         * @brief Get the radius of the sphere of every individual bump bond (union solid with cylinder)
         * @return Radius of bump bond sphere
         */
        double getBumpSphereRadius() const { return bump_sphere_radius_; }
        /**
         * @brief Get the radius of the cylinder of every individual bump bond  (union solid with sphere)
         * @return Radius of bump bond cylinder
         */
        double getBumpCylinderRadius() const { return bump_cylinder_radius_; }
        /**
         * @brief Get the height of the bump bond cylinder, determining the offset between sensor and chip
         * @return Height of the bump bonds
         */
        double getBumpHeight() const { return bump_height_; }

    private:
        double bump_sphere_radius_{};
        double bump_height_{};
        ROOT::Math::XYVector bump_offset_;
        double bump_cylinder_radius_{};
    };

    /**
     * A monolithic detector assembly describing a setup where sensor and readout ASIC consist of a single slab of silicon
     */
    class MonolithicAssembly : public DetectorAssembly {
    public:
        /**
         * Constructor for monolithic assemblies
         * @param header_config Configuration reference holding the unnamed section fo detector configuration
         */
        explicit MonolithicAssembly(Configuration& header_config) : DetectorAssembly(header_config) {

            // Excess around the chip is copied from sensor size
            auto default_assembly_excess = header_config.get<double>("sensor_excess", 0);
            excess_.at(0) = header_config.get<double>("sensor_excess_top", default_assembly_excess);
            excess_.at(1) = header_config.get<double>("sensor_excess_right", default_assembly_excess);
            excess_.at(2) = header_config.get<double>("sensor_excess_bottom", default_assembly_excess);
            excess_.at(3) = header_config.get<double>("sensor_excess_left", default_assembly_excess);
        }
    };
} // namespace allpix

#endif // ALLPIX_SUPPORT_LAYER_H
