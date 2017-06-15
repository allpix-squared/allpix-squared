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
            : DetectorModel(std::move(type)), number_of_pixels_(1, 1), coverlayer_material_("Al"), has_coverlayer_(false) {}

        /* Number of pixels */
        ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> getNPixels() const override {
            return number_of_pixels_;
        }
        void setNPixels(ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> val) {
            number_of_pixels_ = std::move(val);
        }

        /* Pixel dimensions */
        ROOT::Math::XYVector getPixelSize() const override { return pixel_size_; }
        void setPixelSize(ROOT::Math::XYVector val) { pixel_size_ = std::move(val); }

        /* Sensor offset */
        // FIXME: a PCB offset makes probably far more sense
        ROOT::Math::XYVector getSensorOffset() const { return sensor_offset_; }
        void setSensorOffset(ROOT::Math::XYVector val) { sensor_offset_ = std::move(val); }

        /* Chip dimensions */
        ROOT::Math::XYZVector getChipSize() const { return chip_size_; }
        void setChipSize(ROOT::Math::XYZVector val) { chip_size_ = std::move(val); }
        /* Chip offset */
        ROOT::Math::XYZVector getChipOffset() const { return chip_offset_; }
        void setChipOffset(ROOT::Math::XYZVector val) { chip_offset_ = std::move(val); }

        /* PCB dimensions */
        ROOT::Math::XYZVector getPCBSize() const { return pcb_size_; }
        void setPCBSize(ROOT::Math::XYZVector val) { pcb_size_ = std::move(val); }

        /* Bump bonds */
        double getBumpSphereRadius() const { return bump_sphere_radius_; }
        void setBumpSphereRadius(double val) { bump_sphere_radius_ = val; }
        double getBumpHeight() const { return bump_height_; }
        void setBumpHeight(double val) { bump_height_ = val; }
        ROOT::Math::XYVector getBumpOffset() const { return bump_offset_; }
        void setBumpOffset(ROOT::Math::XYVector val) { bump_offset_ = std::move(val); }
        double getBumpCylinderRadius() const { return bump_cylinder_radius_; }
        void setBumpCylinderRadius(double val) { bump_cylinder_radius_ = val; }

        /* Guard rings */
        double getGuardRingExcessTop() const { return guard_ring_excess_top_; }
        void setGuardRingExcessTop(double val) { guard_ring_excess_top_ = val; }
        double getGuardRingExcessBottom() const { return guard_ring_excess_bottom_; }
        void setGuardRingExcessBottom(double val) { guard_ring_excess_bottom_ = val; }
        double getGuardRingExcessRight() const { return guard_ring_excess_right_; }
        void setGuardRingExcessRight(double val) { guard_ring_excess_right_ = val; }
        double getGuardRingExcessLeft() const { return guard_ring_excess_left_; }
        void getGuardRingExcessLeft(double val) { guard_ring_excess_left_ = val; }

        /* Coverlayer */
        bool hasCoverlayer() const { return has_coverlayer_; }
        double getCoverlayerHeight() const { return coverlayer_height_; }
        void setCoverlayerHeight(double val) {
            coverlayer_height_ = val;
            has_coverlayer_ = true;
        }

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

        std::string getCoverlayerMaterial() const { return coverlayer_material_; }
        void setCoverlayerMaterial(std::string mat) { coverlayer_material_ = std::move(mat); }

    private:
        ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> number_of_pixels_;

        ROOT::Math::XYVector pixel_size_;

        ROOT::Math::XYVector sensor_offset_;

        ROOT::Math::XYZVector chip_size_;
        ROOT::Math::XYZVector chip_offset_;

        ROOT::Math::XYZVector pcb_size_;

        double bump_sphere_radius_{};
        double bump_height_{};
        ROOT::Math::XYVector bump_offset_;
        double bump_cylinder_radius_{};

        // FIXME: use a 4D vector here?
        double guard_ring_excess_top_{};
        double guard_ring_excess_bottom_{};
        double guard_ring_excess_right_{};
        double guard_ring_excess_left_{};

        double coverlayer_height_{};
        std::string coverlayer_material_;
        bool has_coverlayer_;
    };
} // namespace allpix

#endif /* ALLPIX_PIXEL_DETECTOR_H */
