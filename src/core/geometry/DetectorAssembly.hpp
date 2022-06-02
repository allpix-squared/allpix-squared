/**
 * @file
 * @brief Definition of detector assemblies
 *
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_DETECTOR_ASSEMBLY_H
#define ALLPIX_DETECTOR_ASSEMBLY_H

#include <Math/Point2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

/**
 * @brief Helper class to hold information on the detector assembly
 */
namespace allpix {
    class DetectorAssembly {
    public:
        DetectorAssembly(const ConfigReader& reader) {
            auto config = reader.getHeaderConfiguration();

            // Chip thickness
            thickness_ = config.get<double>("assembly_thickness", 0);
        }
        DetectorAssembly() = delete;
        virtual ~DetectorAssembly() = default;

        /**
         * @brief Get the thickness of the chip
         * @return Thickness of the chip
         */
        double getThickness() const { return thickness_; }

        ROOT::Math::XYVector getExcess() const { return {(excess_.at(1) + excess_.at(3)), (excess_.at(0) + excess_.at(2))}; }

        virtual ROOT::Math::XYZVector getOffset() const {
            return {(excess_.at(1) - excess_.at(3)), (excess_.at(0) - excess_.at(2)), 0};
        }

    protected:
        std::array<double, 4> excess_{};

    private:
        double thickness_{};
    };

    class HybridAssembly : public DetectorAssembly {
    public:
        HybridAssembly(const ConfigReader& reader) : DetectorAssembly(reader) {
            auto config = reader.getHeaderConfiguration();

            // Excess around the chip from the pixel grid
            auto default_assembly_excess = config.get<double>("assembly_excess", 0);
            excess_.at(0) = config.get<double>("assembly_excess_top", default_assembly_excess);
            excess_.at(1) = config.get<double>("assembly_excess_right", default_assembly_excess);
            excess_.at(2) = config.get<double>("assembly_excess_bottom", default_assembly_excess);
            excess_.at(3) = config.get<double>("assembly_excess_left", default_assembly_excess);

            // Set bump parameters
            bump_cylinder_radius_ = config.get<double>("bump_cylinder_radius");
            bump_height_ = config.get<double>("bump_height");
            bump_sphere_radius_ = config.get<double>("bump_sphere_radius", 0);

            auto pitch = config.get<ROOT::Math::XYVector>("pixel_size");
            bump_offset_ = config.get<ROOT::Math::XYVector>("bump_offset", {0, 0});
            if(std::fabs(bump_offset_.x()) > pitch.x() / 2.0 || std::fabs(bump_offset_.y()) > pitch.y() / 2.0) {
                throw InvalidValueError(config, "bump_offset", "bump bond offset cannot be larger than half pixel pitch");
            }
        }

        ROOT::Math::XYZVector getOffset() const override {
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

    class MonolithicAssembly : public DetectorAssembly {
    public:
        MonolithicAssembly(const ConfigReader& reader) : DetectorAssembly(reader) {
            auto config = reader.getHeaderConfiguration();

            // Excess around the chip is copied from sensor size
            auto default_assembly_excess = config.get<double>("sensor_excess", 0);
            excess_.at(0) = config.get<double>("sensor_excess_top", default_assembly_excess);
            excess_.at(1) = config.get<double>("sensor_excess_right", default_assembly_excess);
            excess_.at(2) = config.get<double>("sensor_excess_bottom", default_assembly_excess);
            excess_.at(3) = config.get<double>("sensor_excess_left", default_assembly_excess);
        }

    private:
    };
} // namespace allpix

#endif // ALLPIX_SUPPORT_LAYER_H
