/**
 * @file
 * @brief Parameters of a pixel detector model
 *
 * @copyright MIT License
 */

#ifndef ALLPIX_PIXEL_DETECTOR_H
#define ALLPIX_PIXEL_DETECTOR_H

#include <string>
#include <utility>

#include <Math/Cartesian2D.h>
#include <Math/DisplacementVector2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

#include "DetectorModel.hpp"

#include "core/utils/log.h"

// TODO [doc] This class is fully documented later when it is cleaned up further

namespace allpix {

    /**
     * @ingroup DetectorModels
     * @brief Model of a hybrid pixel detector: a detector where the sensor is bump-bonded to the chip
     */
    class HybridPixelDetectorModel : public DetectorModel {
    public:
        // Constructor and destructor
        explicit HybridPixelDetectorModel(std::string type)
            : DetectorModel(std::move(type)), coverlayer_material_("Al"), has_coverlayer_(false) {}

        /**
         * @brief Get center of the chip in local coordinates
         * @return Center of the chip
         *
         * Defaults to the center of the grid as given by \ref Detector::getCenter() with sensor offset.
         */
        virtual ROOT::Math::XYZPoint getChipCenter() const override {
            ROOT::Math::XYZVector offset((chip_excess_[1] - chip_excess_[3]) / 2.0,
                                         (chip_excess_[0] - chip_excess_[2]) / 2.0,
                                         -getSensorSize().z() / 2.0 - getBumpHeight() - getChipSize().z() / 2.0);
            return getCenter() + offset;
        }

        /**
         * @brief Get center of the chip in local coordinates
         * @return Center of the chip
         */
        virtual ROOT::Math::XYZPoint getPCBCenter() const override {
            ROOT::Math::XYZVector offset(
                0, 0, -getSensorSize().z() / 2.0 - getBumpHeight() - getChipSize().z() - getPCBSize().z() / 2.0);
            return getCenter() + offset;
        }

        /* Bump bonds */
        virtual ROOT::Math::XYZPoint getBumpsCenter() const {
            ROOT::Math::XYZVector offset(0, 0, -getSensorSize().z() / 2.0 - getBumpHeight() / 2.0);
            return getCenter() + offset;
        }
        double getBumpSphereRadius() const { return bump_sphere_radius_; }
        void setBumpSphereRadius(double val) { bump_sphere_radius_ = val; }
        double getBumpCylinderRadius() const { return bump_cylinder_radius_; }
        void setBumpCylinderRadius(double val) { bump_cylinder_radius_ = val; }
        double getBumpHeight() const { return bump_height_; }
        void setBumpHeight(double val) { bump_height_ = val; }
        ROOT::Math::XYVector getBumpOffset() const { return bump_offset_; }
        void setBumpOffset(ROOT::Math::XYVector val) { bump_offset_ = std::move(val); }

        /* Coverlayer */
        bool hasCoverlayer() const { return has_coverlayer_; }
        double getCoverlayerHeight() const { return coverlayer_height_; }
        void setCoverlayerHeight(double val) {
            coverlayer_height_ = val;
            has_coverlayer_ = true;
        }
        std::string getCoverlayerMaterial() const { return coverlayer_material_; }
        void setCoverlayerMaterial(std::string mat) { coverlayer_material_ = std::move(mat); }

        /* FIXME: This will be reworked */
        double getHalfWrapperDX() const { return getPCBSize().x() / 2.0; }
        double getHalfWrapperDY() const { return getPCBSize().y() / 2.0; }
        double getHalfWrapperDZ() const {

            double whdz =
                getPCBSize().z() / 2.0 + getChipSize().z() / 2.0 + getBumpHeight() / 2.0 + getSensorSize().z() / 2.0;

            if(has_coverlayer_) {
                whdz += getCoverlayerHeight() / 2.0;
            }

            return whdz;
        }

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

#endif /* ALLPIX_PIXEL_DETECTOR_H */
