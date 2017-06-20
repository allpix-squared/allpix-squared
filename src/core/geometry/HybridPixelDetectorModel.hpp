/**
 * @file
 * @brief Parameters of a hybrid pixel detector model
 *
 * @copyright MIT License
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

#include "DetectorModel.hpp"

namespace allpix {

    /**
     * @ingroup DetectorModels
     * @brief Model of a hybrid pixel detector. This a model where the sensor is bump-bonded to the chip
     */
    class HybridPixelDetectorModel : public DetectorModel {
    public:
        // Constructor and destructor
        explicit HybridPixelDetectorModel(const Configuration& config)
            : DetectorModel(config), coverlayer_material_("Al"), has_coverlayer_(false) {
            // Set bump parameters
            setBumpCylinderRadius(config.get<double>("bump_cylinder_radius"));
            setBumpHeight(config.get<double>("bump_height"));
            setBumpSphereRadius(config.get<double>("bump_sphere_radius", 0));
            setBumpOffset(config.get<ROOT::Math::XYVector>("bump_offset", {0, 0}));
        }

        /**
         * @brief Get center of the chip in local coordinates
         * @return Center of the chip
         *
         * The center of the chip as given by \ref DetectorModel::getChipCenter() with extra offset for bump bonds.
         */
        virtual ROOT::Math::XYZPoint getChipCenter() const override {
            ROOT::Math::XYZVector offset(0, 0, -getBumpHeight());
            return DetectorModel::getChipCenter() + offset;
        }

        /**
         * @brief Get center of the PCB in local coordinates
         * @return Center of the PCB
         *
         * The center of the chip as given by \ref DetectorModel::getPCBCenter() with extra offset for bump bonds.
         */
        virtual ROOT::Math::XYZPoint getPCBCenter() const override {
            ROOT::Math::XYZVector offset(0, 0, -getBumpHeight());
            return DetectorModel::getPCBCenter() + offset;
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
                bump_offset_.x(), bump_offset_.y(), -getSensorSize().z() / 2.0 - getBumpHeight() / 2.0);
            return getCenter() + offset;
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

        // FIXME: Coverlayer will possibly be revised
        /**
         * @brief Returns if this detector model has a cover layer
         * @return If the model has a cover layer
         */
        bool hasCoverlayer() const { return has_coverlayer_; }
        /**
         * @brief Get the height of the cover layer
         * @return Height of the cover layer
         */
        double getCoverlayerHeight() const { return coverlayer_height_; }
        /**
         * @brief Enables the cover layer and sets the height of the cover layer
         * @param val Height of the cover layer
         */
        void setCoverlayerHeight(double val) {
            coverlayer_height_ = val;
            has_coverlayer_ = true;
        }
        /**
         * @brief Get the material of the cover layer. Default is "Al", thus aluminium.
         * @return Material of the cover layer
         */
        std::string getCoverlayerMaterial() const { return coverlayer_material_; }
        /**
         * @brief Set the material of the cover layer. Should represent element in periodic system.
         * @param val Material of the cover layer
         */
        void setCoverlayerMaterial(std::string mat) { coverlayer_material_ = std::move(mat); }

    private:
        double bump_sphere_radius_{};
        double bump_height_{};
        ROOT::Math::XYVector bump_offset_;
        double bump_cylinder_radius_{};

        double coverlayer_height_{};
        std::string coverlayer_material_;
        bool has_coverlayer_;
    };
} // namespace allpix

#endif /* ALLPIX_HYBRID_PIXEL_DETECTOR_H */
