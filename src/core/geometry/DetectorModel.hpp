/**
 * @file
 * @brief Base of detector models
 *
 * @copyright MIT License
 */

/**
 * @defgroup DetectorModels Detector models
 * @brief Collection of global detector models supported by the framework
 */

#ifndef ALLPIX_GEOMETRY_DETECTOR_MODEL_H
#define ALLPIX_GEOMETRY_DETECTOR_MODEL_H

#include <string>
#include <utility>

#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

namespace allpix {
    /**
     * @ingroup DetectorModels
     * @brief Base of all detector models
     *
     * Implements the minimum required for a detector model. A model always has a sensitive device (referred as the sensor).
     * This sensor has the following properties:
     * - A size in three dimensions
     * - A set of two dimensional replicated blocks called pixels
     * - An offset of the sensor minimum from the center of the first pixel (the origin in the local coordinate system)
     * - The coordinates of the position that is used to place the detector in the global frame (the rotation center)
     */
    class DetectorModel {
    public:
        /**
         * @brief Constructs a detector model of a certain type
         * @param type Unique type description of a model
         */
        explicit DetectorModel(std::string type) : type_(std::move(type)), number_of_pixels_(1, 1) {}
        /**
         * @brief Essential virtual destructor
         */
        virtual ~DetectorModel() = default;

        ///@{
        /**
         * @brief Use default copy and move behaviour
         */
        DetectorModel(const DetectorModel&) = default;
        DetectorModel& operator=(const DetectorModel&) = default;

        DetectorModel(DetectorModel&&) = default;
        DetectorModel& operator=(DetectorModel&&) = default;
        ///@}

        /**
         * @brief Get the type of the model
         * @return Model type
         */
        std::string getType() const { return type_; }

        /**
         * @brief Get coordinate of center of chip in local frame of derived model
         * @note It can be a bit counter intuitive that this is not always (0, 0, 0)
         *
         * The center coordinate corresponds to the \ref Detector::getPosition "position" in the global frame.
         */
        virtual ROOT::Math::XYZPoint getCenter() const {
            return ROOT::Math::XYZPoint(getSensorSize().x() / 2.0 - getPixelSize().x() / 2.0,
                                        getSensorSize().y() / 2.0 - getPixelSize().y() / 2.0,
                                        0);
        }

        /** PIXEL GRID **/
        /**
         * @brief Get number of pixel (replicated blocks in generic sensors)
         * @return Number of two dimensional pixels
         */
        virtual ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> getNPixels() const {
            return number_of_pixels_;
        }
        /**
         * @brief Set number of pixels (replicated blocks in generic sensors)
         * @param val Number of two dimensional pixels
         */
        void setNPixels(ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> val) {
            number_of_pixels_ = std::move(val);
        }
        /**
         * @brief Get size of a single pixel
         * @return Size of a pixel
         */
        virtual ROOT::Math::XYVector getPixelSize() const { return pixel_size_; }
        /**
         * @brief Set the size of a pixel
         * @param val Size of a pixel
         */
        void setPixelSize(ROOT::Math::XYVector val) { pixel_size_ = std::move(val); }
        /**
         * @brief Get total size of the pixel grid
         * @param val Size of the pixel grid
         *
         * @warning The grid has zero thickness
         * @note This is basically a 2D method, but provided in 3D because it is primarily used there
         */
        ROOT::Math::XYZVector getGridSize() const {
            return ROOT::Math::XYZVector(getNPixels().x() * getPixelSize().x(), getNPixels().y() * getPixelSize().y(), 0);
        }

        /** SENSOR **/
        /**
         * @brief Get size of the sensor
         * @return Size of the sensor
         *
         * Calculated from \ref Detector::getGridSize "pixel grid size", sensor excess and sensor thickness
         */
        virtual ROOT::Math::XYZVector getSensorSize() const {
            ROOT::Math::XYZVector excess_thickness(
                (sensor_excess_[1] + sensor_excess_[3]), (sensor_excess_[0] + sensor_excess_[2]), sensor_thickness_);
            return getGridSize() + excess_thickness;
        }
        /**
         * @brief Get center of the sensor in local coordinates
         * @return Center of the sensor
         *
         * Center of the model with excess taken into account
         */
        virtual ROOT::Math::XYZPoint getSensorCenter() const {
            ROOT::Math::XYZVector offset(
                (sensor_excess_[1] - sensor_excess_[3]) / 2.0, (sensor_excess_[0] - sensor_excess_[2]) / 2.0, 0);
            return getCenter() + offset;
        }
        /**
         * @brief Set the thickness of the sensor
         * @param val Thickness of the sensor
         */
        void setSensorThickness(double val) { sensor_thickness_ = val; }
        /* FIXME: sensor params */
        void setSensorExcessTop(double val) { sensor_excess_[0] = val; }
        void setSensorExcessRight(double val) { sensor_excess_[1] = val; }
        void setSensorExcessBottom(double val) { sensor_excess_[2] = val; }
        void setSensorExcessLeft(double val) { sensor_excess_[3] = val; }

        /** CHIP **/
        /**
         * @brief Get size of the chip
         * @return Size of the chip
         *
         * Calculated from \ref Detector::getGridSize "pixel grid size", chip excess and chip thickness
         */
        virtual ROOT::Math::XYZVector getChipSize() const {
            ROOT::Math::XYZVector excess_thickness(
                (chip_excess_[1] - chip_excess_[3]), (chip_excess_[0] - chip_excess_[2]), chip_thickness_);
            return getGridSize() + excess_thickness;
        }
        /**
         * @brief Get center of the chip in local coordinates
         * @return Center of the chip
         *
         * Defaults to the center of the grid as given by \ref Detector::getCenter() with sensor offset.
         */
        virtual ROOT::Math::XYZPoint getChipCenter() const {
            ROOT::Math::XYZVector offset((chip_excess_[1] - chip_excess_[3]) / 2.0,
                                         (chip_excess_[0] - chip_excess_[2]) / 2.0,
                                         -getSensorSize().z() / 2.0 - getChipSize().z() / 2.0);
            return getCenter() + offset;
        }
        /* FIXME: set chip thickness and excess */
        void setChipThickness(double val) { chip_thickness_ = val; }
        void setChipExcessTop(double val) { chip_excess_[0] = val; }
        void setChipExcessRight(double val) { chip_excess_[1] = val; }
        void setChipExcessBottom(double val) { chip_excess_[2] = val; }
        void setChipExcessLeft(double val) { chip_excess_[3] = val; }

        /** PCB **/
        /**
         * @brief Get size of the PCB
         * @return Size of the PCB
         *
         * Calculated from \ref Detector::getGridSize "pixel grid size", chip excess and chip thickness
         */
        virtual ROOT::Math::XYZVector getPCBSize() const {
            ROOT::Math::XYZVector excess_thickness(
                (pcb_excess_[1] + pcb_excess_[3]), (pcb_excess_[0] + pcb_excess_[2]), pcb_thickness_);
            return getGridSize() + excess_thickness;
        }
        /**
         * @brief Get center of the chip in local coordinates
         * @return Center of the chip
         */
        virtual ROOT::Math::XYZPoint getPCBCenter() const {
            ROOT::Math::XYZVector offset((pcb_excess_[1] - pcb_excess_[3]),
                                         (pcb_excess_[0] - pcb_excess_[2]),
                                         -getSensorSize().z() / 2.0 - getChipSize().z() - getPCBSize().z() / 2.0);
            return getCenter() + offset;
        }
        /* FIXME: set PCB thickness and excess */
        void setPCBThickness(double val) { pcb_thickness_ = val; }
        void setPCBExcessTop(double val) { pcb_excess_[0] = val; }
        void setPCBExcessRight(double val) { pcb_excess_[1] = val; }
        void setPCBExcessBottom(double val) { pcb_excess_[2] = val; }
        void setPCBExcessLeft(double val) { pcb_excess_[3] = val; }

    protected:
        std::string type_;

        ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> number_of_pixels_;
        ROOT::Math::XYVector pixel_size_;

        double sensor_thickness_;
        double sensor_excess_[4]{};

        double chip_thickness_;
        double chip_excess_[4]{};

        double pcb_thickness_;
        double pcb_excess_[4]{};
    };
} // namespace allpix

#endif // ALLPIX_GEOMETRY_MANAGER_H
