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
            : DetectorModel(std::move(type)), number_of_pixels_(1, 1), coverlayer_material_("Al"), has_coverlayer_(false) {}

        /* Number of pixels */
        ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> getNPixels() const override {
            return number_of_pixels_;
        }
        void setNPixels(ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> val) {
            number_of_pixels_ = std::move(val);
        }

        /* Sensor size */
        ROOT::Math::XYZVector getSensorSize() const override {
            ROOT::Math::XYZVector excess_thickness(
                (sensor_excess_[1] + sensor_excess_[3]), (sensor_excess_[0] + sensor_excess_[2]), sensor_thickness_);
            return getGridSize() + excess_thickness;
        }
        /* Sensor center */
        virtual ROOT::Math::XYZPoint getSensorCenter() const override {
            ROOT::Math::XYZVector offset(
                (sensor_excess_[1] - sensor_excess_[3]) / 2.0, (sensor_excess_[0] - sensor_excess_[2]) / 2.0, 0);
            return getCenter() + offset;
        }
        /* Sensor offsets */
        void setSensorExcessTop(double val) { sensor_excess_[0] = val; }
        void setSensorExcessRight(double val) { sensor_excess_[1] = val; }
        void setSensorExcessBottom(double val) { sensor_excess_[2] = val; }
        void setSensorExcessLeft(double val) { sensor_excess_[3] = val; }

        /* Chip size */
        ROOT::Math::XYZVector getChipSize() const override {
            ROOT::Math::XYZVector excess_thickness(
                (chip_excess_[1] - chip_excess_[3]), (chip_excess_[0] - chip_excess_[2]), chip_thickness_);
            return getGridSize() + excess_thickness;
        }
        /* Chip center */
        virtual ROOT::Math::XYZPoint getChipCenter() const override {
            ROOT::Math::XYZVector offset((chip_excess_[1] - chip_excess_[3]) / 2.0,
                                         (chip_excess_[0] - chip_excess_[2]) / 2.0,
                                         -getSensorSize().z() / 2.0 - getBumpHeight() - getChipSize().z() / 2.0);
            return getCenter() + offset;
        }
        /* Chip thickness and excess */
        void setChipThickness(double val) { chip_thickness_ = val; }
        void setChipExcessTop(double val) { chip_excess_[0] = val; }
        void setChipExcessRight(double val) { chip_excess_[1] = val; }
        void setChipExcessBottom(double val) { chip_excess_[2] = val; }
        void setChipExcessLeft(double val) { chip_excess_[3] = val; }

        /* PCB size */
        ROOT::Math::XYZVector getPCBSize() const override {
            ROOT::Math::XYZVector excess_thickness(
                (pcb_excess_[1] + pcb_excess_[3]), (pcb_excess_[0] + pcb_excess_[2]), pcb_thickness_);
            return getGridSize() + excess_thickness;
        }
        /* PCB center */
        virtual ROOT::Math::XYZPoint getPCBCenter() const override {
            ROOT::Math::XYZVector offset((pcb_excess_[1] - pcb_excess_[3]) / 2.0,
                                         (pcb_excess_[0] - pcb_excess_[2]) / 2.0,
                                         -getSensorSize().z() / 2.0 - getBumpHeight() - getChipSize().z() -
                                             getPCBSize().z() / 2.0);
            return getCenter() + offset;
        }
        /* set PCB thickness and excess */
        void setPCBThickness(double val) { pcb_thickness_ = val; }
        void setPCBExcessTop(double val) { pcb_excess_[0] = val; }
        void setPCBExcessRight(double val) { pcb_excess_[1] = val; }
        void setPCBExcessBottom(double val) { pcb_excess_[2] = val; }
        void setPCBExcessLeft(double val) { pcb_excess_[3] = val; }

        /* Bump bonds */
        double getBumpSphereRadius() const { return bump_sphere_radius_; }
        void setBumpSphereRadius(double val) { bump_sphere_radius_ = val; }
        double getBumpHeight() const { return bump_height_; }
        void setBumpHeight(double val) { bump_height_ = val; }
        ROOT::Math::XYVector getBumpOffset() const { return bump_offset_; }
        void setBumpOffset(ROOT::Math::XYVector val) { bump_offset_ = std::move(val); }
        double getBumpCylinderRadius() const { return bump_cylinder_radius_; }
        void setBumpCylinderRadius(double val) { bump_cylinder_radius_ = val; }

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
        ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> number_of_pixels_;

        ROOT::Math::XYVector pixel_size_;

        double sensor_excess_[4]{};

        double chip_thickness_;
        double chip_excess_[4]{};

        double pcb_thickness_;
        double pcb_excess_[4]{};

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
