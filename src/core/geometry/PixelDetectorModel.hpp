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
            : DetectorModel(type), number_of_pixels_(1, 1), pixel_size_()
        /*m_sensor_hx(0.0), m_sensor_hy(0.0), m_sensor_hz(0.0), m_sensor_posx(0.0), m_sensor_posy(0.0),
              m_sensor_posz(0.0), m_sensor_gr_excess_htop(0.0), m_sensor_gr_excess_hbottom(0.0),
              m_sensor_gr_excess_hright(0.0), m_sensor_gr_excess_hleft(0.0), m_chip_hx(0.0), m_chip_hy(0.0), m_chip_hz(0.0),
              m_chip_offsetx(0.0), m_chip_offsety(0.0), m_chip_offsetz(0.0), m_chip_posx(0.0), m_chip_posy(0.0),
              m_chip_posz(0.0), m_pcb_hx(0.0), m_pcb_hy(0.0), m_pcb_hz(0.0), m_bump_radius(0.0), m_bump_height(0.0),
              m_bump_offsetx(0.0), m_bump_offsety(0.0), m_bump_dr(0.0), m_coverlayer_hz(0.0), m_coverlayer_mat("Al"),
              m_coverlayer_ON(false)*/ {}
        ~PixelDetectorModel() override = default;

        /* Number of pixels */
        ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> getNPixels() { return number_of_pixels_; }
        inline int GetNPixelsX() { return number_of_pixels_.x(); };
        inline int GetNPixelsY() { return number_of_pixels_.y(); };
        inline int GetNPixelsTotXY() { return GetNPixelsX() * GetNPixelsY(); }; // Planar layout

        void setNPixels(ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> val) {
            number_of_pixels_ = std::move(val);
        }
        void SetNPixelsX(int val) { number_of_pixels_.SetX(val); };
        void SetNPixelsY(int val) { number_of_pixels_.SetY(val); };

        // inline int GetHalfNPixelsX(){return GetNPixelsX()/2;};
        // inline int GetHalfNPixelsY(){return GetNPixelsY()/2;};
        // inline int GetHalfNPixelsZ(){return GetNPixelsZ()/2;};

        /* Pixel dimensions */
        inline ROOT::Math::XYZVector getPixelSize() { return pixel_size_; }
        inline double GetPixelX() { return pixel_size_.x(); };
        inline double GetPixelY() { return pixel_size_.y(); };
        inline double GetPixelZ() { return pixel_size_.z(); }; // FIXME: REMOVE

        inline double GetHalfPixelX() { return pixel_size_.x() / 2.0; };
        inline double GetHalfPixelY() { return pixel_size_.y() / 2.0; };
        inline double GetHalfPixelZ() { return pixel_size_.z() / 2.0; }; // FIXME: REMOVE

        void setPixelSize(ROOT::Math::XYZVector val) { pixel_size_ = std::move(val); }
        void SetPixSizeX(double val) { pixel_size_.SetX(val); }
        void SetPixSizeY(double val) { pixel_size_.SetY(val); }
        void SetPixSizeZ(double val) { pixel_size_.SetZ(val); }

        /* Sensor dimensions */
        inline ROOT::Math::XYZVector getSensorSize() { return sensor_size_; }
        inline double GetSensorX() { return sensor_size_.x(); };
        inline double GetSensorY() { return sensor_size_.y(); };
        inline double GetSensorZ() { return sensor_size_.z(); };

        inline double GetHalfSensorX() { return sensor_size_.x() / 2.0; };
        inline double GetHalfSensorY() { return sensor_size_.y() / 2.0; };
        inline double GetHalfSensorZ() { return sensor_size_.z() / 2.0; };

        void setSensorSize(ROOT::Math::XYZVector val) { sensor_size_ = std::move(val); }
        void SetSensorHX(double val) { sensor_size_.SetX(val); }
        void SetSensorHY(double val) { sensor_size_.SetY(val); }
        void SetSensorHZ(double val) { sensor_size_.SetZ(val); }

        /* Sensor offset */
        inline ROOT::Math::XYVector getSensorOffset() { return sensor_offset_; }
        inline double GetSensorXOffset() { return sensor_offset_.x(); };
        inline double GetSensorYOffset() { return sensor_offset_.y(); };
        inline double GetSensorZOffset() { return GetHalfPCBZ(); }; // FIXME: see relation with GetHalfWrapperDZ()

        void setSensorOffset(ROOT::Math::XYVector val) { sensor_offset_ = std::move(val); }
        void SetSensorPosX(double val) { sensor_offset_.SetX(val); }
        void SetSensorPosY(double val) { sensor_offset_.SetY(val); }
        void SetSensorPosZ(double) {} // FIXME: remove

        /* Chip dimensions */
        inline ROOT::Math::XYZVector getChipSize() { return chip_size_; }
        inline double GetChipX() { return chip_size_.x(); };
        inline double GetChipY() { return chip_size_.y(); };
        inline double GetChipZ() { return chip_size_.z(); };

        inline double GetHalfChipX() { return chip_size_.x() / 2.0; };
        inline double GetHalfChipY() { return chip_size_.y() / 2.0; };
        inline double GetHalfChipZ() { return chip_size_.z() / 2.0; };

        void setChipSize(ROOT::Math::XYZVector val) { chip_size_ = std::move(val); }
        void SetChipHX(double val) { chip_size_.SetX(val); }
        void SetChipHY(double val) { chip_size_.SetY(val); }
        void SetChipHZ(double val) { chip_size_.SetZ(val); }

        /* Chip offset */
        inline ROOT::Math::XYZVector getChipOffset() { return chip_offset_; }
        inline double GetChipXOffset() { return chip_offset_.x(); };
        inline double GetChipYOffset() { return chip_offset_.y(); };
        inline double GetChipZOffset() { return chip_offset_.z(); }; // FIXME: see relation with GetHalfWrapperDZ()

        // void SetChipPosX(double val) { m_chip_posx = val; };
        // void SetChipPosY(double val) { m_chip_posy = val; }
        // void SetChipPosZ(double val) { m_chip_posz = val; };

        void setChipOffset(ROOT::Math::XYZVector val) { chip_offset_ = std::move(val); }
        void SetChipOffsetX(double val) { chip_offset_.SetX(val); };
        void SetChipOffsetY(double val) { chip_offset_.SetY(val); }
        void SetChipOffsetZ(double val) { chip_offset_.SetZ(val); };

        /* PCB dimensions */
        inline ROOT::Math::XYZVector getPCBSize() { return pcb_size_; }
        inline double GetPCBX() { return pcb_size_.x(); };
        inline double GetPCBY() { return pcb_size_.y(); };
        inline double GetPCBZ() { return pcb_size_.z(); };

        inline double GetHalfPCBX() { return pcb_size_.x() / 2.0; };
        inline double GetHalfPCBY() { return pcb_size_.y() / 2.0; };
        inline double GetHalfPCBZ() { return pcb_size_.z() / 2.0; };

        void setPCBSize(ROOT::Math::XYZVector val) { pcb_size_ = std::move(val); }
        void SetPCBHX(double val) { pcb_size_.SetX(val); }
        void SetPCBHY(double val) { pcb_size_.SetY(val); }
        void SetPCBHZ(double val) { pcb_size_.SetZ(val); }

        /* Bump bonds */
        inline double GetBumpRadius() { return m_bump_radius; };
        inline double GetBumpHeight() { return m_bump_height; };
        inline double GetBumpHalfHeight() { return m_bump_height / 2.0; };

        void SetBumpRadius(double val) { m_bump_radius = val; };
        void SetBumpHeight(double val) { m_bump_height = val; };

        inline double GetBumpOffsetX() { return m_bump_offsetx; };
        inline double GetBumpOffsetY() { return m_bump_offsety; };

        void SetBumpOffsetX(double val) { m_bump_offsetx = val; };
        void SetBumpOffsetY(double val) { m_bump_offsety = val; };

        inline double GetBumpDr() { return m_bump_dr; };
        void SetBumpDr(double val) { m_bump_dr = val; };

        /* Guard rings */
        inline double GetSensorExcessHTop() { return m_sensor_gr_excess_htop; };
        inline double GetSensorExcessHBottom() { return m_sensor_gr_excess_hbottom; };
        inline double GetSensorExcessHRight() { return m_sensor_gr_excess_hright; };
        inline double GetSensorExcessHLeft() { return m_sensor_gr_excess_hleft; };

        void SetSensorExcessHTop(double val) { m_sensor_gr_excess_htop = val; }
        void SetSensorExcessHBottom(double val) { m_sensor_gr_excess_hbottom = val; }
        void SetSensorExcessHRight(double val) { m_sensor_gr_excess_hright = val; }
        void SetSensorExcessHLeft(double val) { m_sensor_gr_excess_hleft = val; }

        /* Wrapper calculations (FIXME: this is broken) */
        inline double GetHalfWrapperDX() { return GetHalfPCBX(); };
        inline double GetHalfWrapperDY() { return GetHalfPCBY(); };
        inline double GetHalfWrapperDZ() {

            double whdz = GetHalfPCBZ() + GetHalfChipZ() + GetBumpHalfHeight() + GetHalfSensorZ();

            if(m_coverlayer_ON) {
                whdz += GetHalfCoverlayerZ();
            }

            return whdz;
        };

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

        ROOT::Math::XYZVector pixel_size_; // FIXME: 2d vector!

        ROOT::Math::XYZVector sensor_size_;
        ROOT::Math::XYVector sensor_offset_;

        ROOT::Math::XYZVector chip_size_;
        ROOT::Math::XYZVector chip_offset_;

        ROOT::Math::XYZVector pcb_size_;

        double m_bump_radius;
        double m_bump_height;
        double m_bump_offsetx;
        double m_bump_offsety;
        double m_bump_dr;

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
