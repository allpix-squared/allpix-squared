/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_PIXEL_DETECTOR_H
#define ALLPIX_PIXEL_DETECTOR_H

#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <tuple>

#include <Math/Cartesian2D.h>
#include <Math/DisplacementVector2D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

#include "DetectorModel.hpp"

// FIXME: this needs a restructure...
namespace allpix {

    class PixelDetectorModel : public DetectorModel {
    public:
        // Constructor and destructor
        explicit PixelDetectorModel(std::string type)
            : DetectorModel(type), number_of_pixels_(1, 1), pixel_size_(), sensor_size_(), sensor_offset_(), chip_size_(),
              chip_offset_(), pcb_size_(), bump_sphere_radius_(0.0), bump_height_(0.0), bump_offset_(),
              bump_cylinder_radius_(0.0), m_coverlayer_hz(0.0), m_coverlayer_mat("Al"), m_coverlayer_ON(false) {}
        ~PixelDetectorModel() override = default;

        /* Number of pixels */
        ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> getNPixels() { return number_of_pixels_; }
        inline int GetNPixelsX() { return number_of_pixels_.x(); };
        inline int GetNPixelsY() { return number_of_pixels_.y(); };

        void setNPixels(ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> val) {
            number_of_pixels_ = std::move(val);
        }
        void SetNPixelsX(int val) { number_of_pixels_.SetX(val); };
        void SetNPixelsY(int val) { number_of_pixels_.SetY(val); };

        /* Pixel dimensions */
        inline ROOT::Math::XYVector getPixelSize() { return pixel_size_; }
        inline double GetPixelX() { return pixel_size_.x(); };
        inline double GetPixelY() { return pixel_size_.y(); };

        inline double GetHalfPixelX() { return pixel_size_.x() / 2.0; };
        inline double GetHalfPixelY() { return pixel_size_.y() / 2.0; };

        void setPixelSize(ROOT::Math::XYVector val) { pixel_size_ = std::move(val); }
        void SetPixelSizeX(double val) { pixel_size_.SetX(val); }
        void SetPixelSizeY(double val) { pixel_size_.SetY(val); }

        /* Sensor dimensions */
        inline ROOT::Math::XYZVector getSensorSize() { return sensor_size_; }
        inline double GetSensorX() { return sensor_size_.x(); };
        inline double GetSensorY() { return sensor_size_.y(); };
        inline double GetSensorZ() { return sensor_size_.z(); };

        inline double GetHalfSensorX() { return sensor_size_.x() / 2.0; };
        inline double GetHalfSensorY() { return sensor_size_.y() / 2.0; };
        inline double GetHalfSensorZ() { return sensor_size_.z() / 2.0; };

        void setSensorSize(ROOT::Math::XYZVector val) { sensor_size_ = std::move(val); }
        void SetSensorSizeX(double val) { sensor_size_.SetX(val); }
        void SetSensorSizeY(double val) { sensor_size_.SetY(val); }
        void SetSensorSizeZ(double val) { sensor_size_.SetZ(val); }

        /* Sensor offset */
        inline ROOT::Math::XYVector getSensorOffset() { return sensor_offset_; }
        inline double GetSensorXOffset() { return sensor_offset_.x(); };
        inline double GetSensorYOffset() { return sensor_offset_.y(); };
        inline double GetSensorZOffset() { return GetHalfPCBZ(); }; // FIXME: see relation with GetHalfWrapperDZ()

        void setSensorOffset(ROOT::Math::XYVector val) { sensor_offset_ = std::move(val); }
        void SetSensorOffsetX(double val) { sensor_offset_.SetX(val); }
        void SetSensorOffsetY(double val) { sensor_offset_.SetY(val); }

        /* Chip dimensions */
        inline ROOT::Math::XYZVector getChipSize() { return chip_size_; }
        inline double GetChipX() { return chip_size_.x(); };
        inline double GetChipY() { return chip_size_.y(); };
        inline double GetChipZ() { return chip_size_.z(); };

        inline double GetHalfChipX() { return chip_size_.x() / 2.0; };
        inline double GetHalfChipY() { return chip_size_.y() / 2.0; };
        inline double GetHalfChipZ() { return chip_size_.z() / 2.0; };

        void setChipSize(ROOT::Math::XYZVector val) { chip_size_ = std::move(val); }
        void SetChipSizeX(double val) { chip_size_.SetX(val); }
        void SetChipSizeY(double val) { chip_size_.SetY(val); }
        void SetChipSizeZ(double val) { chip_size_.SetZ(val); }

        /* Chip offset */
        inline ROOT::Math::XYZVector getChipOffset() { return chip_offset_; }
        inline double GetChipXOffset() { return chip_offset_.x(); };
        inline double GetChipYOffset() { return chip_offset_.y(); };
        inline double GetChipZOffset() { return chip_offset_.z(); };

        void setChipOffset(ROOT::Math::XYZVector val) { chip_offset_ = std::move(val); }
        void SetChipOffsetX(double val) { chip_offset_.SetX(val); }
        void SetChipOffsetY(double val) { chip_offset_.SetY(val); }
        void SetChipOffsetZ(double val) { chip_offset_.SetZ(val); }

        /* PCB dimensions */
        inline ROOT::Math::XYZVector getPCBSize() { return pcb_size_; }
        inline double GetPCBX() { return pcb_size_.x(); }
        inline double GetPCBY() { return pcb_size_.y(); }
        inline double GetPCBZ() { return pcb_size_.z(); }

        inline double GetHalfPCBX() { return pcb_size_.x() / 2.0; };
        inline double GetHalfPCBY() { return pcb_size_.y() / 2.0; };
        inline double GetHalfPCBZ() { return pcb_size_.z() / 2.0; };

        void setPCBSize(ROOT::Math::XYZVector val) { pcb_size_ = std::move(val); }
        void SetPCBSizeX(double val) { pcb_size_.SetX(val); }
        void SetPCBSizeY(double val) { pcb_size_.SetY(val); }
        void SetPCBSizeZ(double val) { pcb_size_.SetZ(val); }

        /* Bump bonds */
        inline double GetBumpRadius() { return bump_sphere_radius_; }
        inline double GetBumpHeight() { return bump_height_; }

        void SetBumpRadius(double val) { bump_sphere_radius_ = val; }
        void SetBumpHeight(double val) { bump_height_ = val; }

        void getBumpOffset(ROOT::Math::XYVector val) { bump_offset_ = std::move(val); }
        inline double GetBumpOffsetX() { return bump_offset_.x(); }
        inline double GetBumpOffsetY() { return bump_offset_.y(); }

        void setBumpOffset(ROOT::Math::XYVector val) { bump_offset_ = std::move(val); }
        void SetBumpOffsetX(double val) { bump_offset_.SetX(val); }
        void SetBumpOffsetY(double val) { bump_offset_.SetY(val); }

        inline double GetBumpDr() { return bump_cylinder_radius_; }
        void SetBumpDr(double val) { bump_cylinder_radius_ = val; }

        /* Guard rings */
        inline double GetSensorExcessHTop() { return m_sensor_gr_excess_htop; }
        inline double GetSensorExcessHBottom() { return m_sensor_gr_excess_hbottom; }
        inline double GetSensorExcessHRight() { return m_sensor_gr_excess_hright; }
        inline double GetSensorExcessHLeft() { return m_sensor_gr_excess_hleft; }

        void SetSensorExcessHTop(double val) { m_sensor_gr_excess_htop = val; }
        void SetSensorExcessHBottom(double val) { m_sensor_gr_excess_hbottom = val; }
        void SetSensorExcessHRight(double val) { m_sensor_gr_excess_hright = val; }
        void SetSensorExcessHLeft(double val) { m_sensor_gr_excess_hleft = val; }

        /* Wrapper calculations (FIXME: this is broken) */
        inline double GetHalfWrapperDX() { return GetHalfPCBX(); }
        inline double GetHalfWrapperDY() { return GetHalfPCBY(); }
        inline double GetHalfWrapperDZ() {

            double whdz = GetHalfPCBZ() + GetHalfChipZ() + GetBumpHeight() / 2.0 + GetHalfSensorZ();

            if(m_coverlayer_ON) {
                whdz += GetHalfCoverlayerZ();
            }

            return whdz;
        }

        /* Coverlayer */
        bool IsCoverlayerON() { return m_coverlayer_ON; }
        double GetHalfCoverlayerZ() { return m_coverlayer_hz; }
        std::string GetCoverlayerMat() { return m_coverlayer_mat; }

        void SetCoverlayerHZ(double val) {
            m_coverlayer_hz = val;
            m_coverlayer_ON = true;
        }
        void SetCoverlayerMat(std::string mat) { m_coverlayer_mat = std::move(mat); }

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
        double m_sensor_gr_excess_htop;
        double m_sensor_gr_excess_hbottom;
        double m_sensor_gr_excess_hright;
        double m_sensor_gr_excess_hleft;

        double m_coverlayer_hz;
        std::string m_coverlayer_mat;
        bool m_coverlayer_ON;
    };
}

#endif /* ALLPIX_PIXEL_DETECTOR_H */
