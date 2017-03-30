/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_PIXEL_DETECTOR_H
#define ALLPIX_PIXEL_DETECTOR_H

#include <string>
#include <utility>

#include <Math/Cartesian2D.h>
#include <Math/DisplacementVector2D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

#include "DetectorModel.hpp"

namespace allpix {

    class PixelDetectorModel : public DetectorModel {
    public:
        // Constructor and destructor
        explicit PixelDetectorModel(std::string type)
            : DetectorModel(type), number_of_pixels_(1, 1), pixel_size_(), sensor_size_(), sensor_offset_(), chip_size_(),
              chip_offset_(), pcb_size_(), bump_sphere_radius_(0.0), bump_height_(0.0), bump_offset_(),
              bump_cylinder_radius_(0.0), guard_ring_excess_top_(), guard_ring_excess_bottom_(), guard_ring_excess_right_(),
              guard_ring_excess_left_(), coverlayer_height_(0.0), coverlayer_material_("Al"), has_coverlayer_(false) {}
        ~PixelDetectorModel() override = default;

        /* Number of pixels */
        ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> getNPixels() { return number_of_pixels_; }
        int getNPixelsX() { return number_of_pixels_.x(); };
        int getNPixelsY() { return number_of_pixels_.y(); };

        void setNPixels(ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> val) {
            number_of_pixels_ = std::move(val);
        }
        void setNPixelsX(int val) { number_of_pixels_.SetX(val); };
        void setNPixelsY(int val) { number_of_pixels_.SetY(val); };

        /* Pixel dimensions */
        ROOT::Math::XYVector getPixelSize() { return pixel_size_; }
        double getPixelSizeX() { return pixel_size_.x(); };
        double getPixelSizeY() { return pixel_size_.y(); };

        double getHalfPixelSizeX() { return pixel_size_.x() / 2.0; };
        double getHalfPixelSizeY() { return pixel_size_.y() / 2.0; };

        void setPixelSize(ROOT::Math::XYVector val) { pixel_size_ = std::move(val); }
        void setPixelSizeX(double val) { pixel_size_.SetX(val); }
        void setPixelSizeY(double val) { pixel_size_.SetY(val); }

        /* Sensor dimensions */
        ROOT::Math::XYZVector getSensorSize() { return sensor_size_; }
        double getSensorSizeX() { return sensor_size_.x(); };
        double getSensorSizeY() { return sensor_size_.y(); };
        double getSensorSizeZ() { return sensor_size_.z(); };

        double getHalfSensorSizeX() { return sensor_size_.x() / 2.0; };
        double getHalfSensorSizeY() { return sensor_size_.y() / 2.0; };
        double getHalfSensorZ() { return sensor_size_.z() / 2.0; };

        void setSensorSize(ROOT::Math::XYZVector val) { sensor_size_ = std::move(val); }
        void setSensorSizeX(double val) { sensor_size_.SetX(val); }
        void setSensorSizeY(double val) { sensor_size_.SetY(val); }
        void setSensorSizeZ(double val) { sensor_size_.SetZ(val); }

        /* Sensor offset */
        ROOT::Math::XYVector getSensorOffset() { return sensor_offset_; }
        double getSensorOffsetX() { return sensor_offset_.x(); };
        double getSensorOffsetY() { return sensor_offset_.y(); };
        double getSensorOffsetZ() { return getHalfPCBSizeZ(); }; // FIXME: see relation with GetHalfWrapperDZ()

        void setSensorOffset(ROOT::Math::XYVector val) { sensor_offset_ = std::move(val); }
        void setSensorOffsetX(double val) { sensor_offset_.SetX(val); }
        void setSensorOffsetY(double val) { sensor_offset_.SetY(val); }

        /* Chip dimensions */
        ROOT::Math::XYZVector getChipSize() { return chip_size_; }
        double getChipSizeX() { return chip_size_.x(); };
        double getChipSizeY() { return chip_size_.y(); };
        double getChipSizeZ() { return chip_size_.z(); };

        double getHalfChipSizeX() { return chip_size_.x() / 2.0; };
        double getHalfChipSizeY() { return chip_size_.y() / 2.0; };
        double getHalfChipSizeZ() { return chip_size_.z() / 2.0; };

        void setChipSize(ROOT::Math::XYZVector val) { chip_size_ = std::move(val); }
        void setChipSizeX(double val) { chip_size_.SetX(val); }
        void setChipSizeY(double val) { chip_size_.SetY(val); }
        void setChipSizeZ(double val) { chip_size_.SetZ(val); }

        /* Chip offset */
        ROOT::Math::XYZVector getChipOffset() { return chip_offset_; }
        double getChipOffsetX() { return chip_offset_.x(); };
        double getChipOffsetY() { return chip_offset_.y(); };
        double getChipOffsetZ() { return chip_offset_.z(); };

        void setChipOffset(ROOT::Math::XYZVector val) { chip_offset_ = std::move(val); }
        void setChipOffsetX(double val) { chip_offset_.SetX(val); }
        void setChipOffsetY(double val) { chip_offset_.SetY(val); }
        void setChipOffsetZ(double val) { chip_offset_.SetZ(val); }

        /* PCB dimensions */
        ROOT::Math::XYZVector getPCBSize() { return pcb_size_; }
        double getPCBSizeX() { return pcb_size_.x(); }
        double getPCBSizeY() { return pcb_size_.y(); }
        double getPCBSizeZ() { return pcb_size_.z(); }

        double getHalfPCBSizeX() { return pcb_size_.x() / 2.0; };
        double getHalfPCBSizeY() { return pcb_size_.y() / 2.0; };
        double getHalfPCBSizeZ() { return pcb_size_.z() / 2.0; };

        void setPCBSize(ROOT::Math::XYZVector val) { pcb_size_ = std::move(val); }
        void setPCBSizeX(double val) { pcb_size_.SetX(val); }
        void setPCBSizeY(double val) { pcb_size_.SetY(val); }
        void setPCBSizeZ(double val) { pcb_size_.SetZ(val); }

        /* Bump bonds */
        double getBumpSphereRadius() { return bump_sphere_radius_; }
        void setBumpSphereRadius(double val) { bump_sphere_radius_ = val; }

        double getBumpHeight() { return bump_height_; }
        void setBumpHeight(double val) { bump_height_ = val; }

        void getBumpOffset(ROOT::Math::XYVector val) { bump_offset_ = std::move(val); }
        double getBumpOffsetX() { return bump_offset_.x(); }
        double getBumpOffsetY() { return bump_offset_.y(); }

        void setBumpOffset(ROOT::Math::XYVector val) { bump_offset_ = std::move(val); }
        void setBumpOffsetX(double val) { bump_offset_.SetX(val); }
        void setBumpOffsetY(double val) { bump_offset_.SetY(val); }

        double getBumpCylinderRadius() { return bump_cylinder_radius_; }
        void setBumpCylinderRadius(double val) { bump_cylinder_radius_ = val; }

        /* Guard rings */
        double getGuardRingExcessTop() { return guard_ring_excess_top_; }
        double getGuardRingExcessBottom() { return guard_ring_excess_bottom_; }
        double getGuardRingExcessRight() { return guard_ring_excess_right_; }
        double getGuardRingExcessLeft() { return guard_ring_excess_left_; }

        void setGuardRingExcessTop(double val) { guard_ring_excess_top_ = val; }
        void setGuardRingExcessBottom(double val) { guard_ring_excess_bottom_ = val; }
        void setGuardRingExcessHRight(double val) { guard_ring_excess_right_ = val; }
        void getGuardRingExcessLeft(double val) { guard_ring_excess_left_ = val; }

        /* Wrapper calculations (FIXME: this has to be reworked...) */
        double getHalfWrapperDX() { return getHalfPCBSizeX(); }
        double getHalfWrapperDY() { return getHalfPCBSizeY(); }
        double getHalfWrapperDZ() {

            double whdz = getHalfPCBSizeZ() + getHalfChipSizeZ() + getBumpHeight() / 2.0 + getHalfSensorZ();

            if(has_coverlayer_) {
                whdz += getHalfCoverlayerHeight();
            }

            return whdz;
        }

        /* Coverlayer */
        bool hasCoverlayer() { return has_coverlayer_; }
        double getCoverlayerHeight() { return coverlayer_height_; }
        double getHalfCoverlayerHeight() { return coverlayer_height_ / 2.0; }
        void setCoverlayerHeight(double val) {
            coverlayer_height_ = val;
            has_coverlayer_ = true;
        }

        std::string getCoverlayerMaterial() { return coverlayer_material_; }
        void setCoverlayerMaterial(std::string mat) { coverlayer_material_ = std::move(mat); }

    private:
        ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> number_of_pixels_;

        ROOT::Math::XYVector pixel_size_;

        ROOT::Math::XYZVector sensor_size_;
        ROOT::Math::XYVector sensor_offset_;

        ROOT::Math::XYZVector chip_size_;
        ROOT::Math::XYZVector chip_offset_;

        ROOT::Math::XYZVector pcb_size_;

        double bump_sphere_radius_;
        double bump_height_;
        ROOT::Math::XYVector bump_offset_;
        double bump_cylinder_radius_;

        // FIXME: use a 4D vector here?
        double guard_ring_excess_top_;
        double guard_ring_excess_bottom_;
        double guard_ring_excess_right_;
        double guard_ring_excess_left_;

        double coverlayer_height_;
        std::string coverlayer_material_;
        bool has_coverlayer_;
    };
} // namespace allpix

#endif /* ALLPIX_PIXEL_DETECTOR_H */
