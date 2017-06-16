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
        explicit DetectorModel(std::string type) : type_(std::move(type)) {}
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
         * @return List of two dimensional pixels
         */
        virtual ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> getNPixels() const {
            return ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>>(1, 1);
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
         * @brief Get total size of the pixel grid (grid has zero thickness)
         * @param val Size of the pixel grid
         *
         * @note This is basically a 2D method, but provided as 3D because it is primarily used for that
         */
        ROOT::Math::XYZVector getGridSize() const {
            return ROOT::Math::XYZVector(getNPixels().x() * getPixelSize().x(), getNPixels().y() * getPixelSize().y(), 0);
        }

        /** SENSOR **/
        /**
         * @brief Get size of the sensor
         * @return Size of the sensor
         */
        virtual ROOT::Math::XYZVector getSensorSize() const {
            return getGridSize() + ROOT::Math::XYZVector(0, 0, sensor_thickness_);
        }
        /**
         * @brief Set the thickness of the sensor
         * @param val Thickness of the sensor
         */
        void setSensorThickness(double val) { sensor_thickness_ = val; }
        /**
         * @brief Get the thickness of the sensor
         * @return Thickness of the sensor
         */
        double getSensorThickness() const { return sensor_thickness_; }
        /**
         * @brief Get center of the sensor in local coordinates
         * @return Center of the sensor
         *
         * Default to the center of the grid as given by \ref Detector::getCenter().
         */
        virtual ROOT::Math::XYZPoint getSensorCenter() const { return getCenter(); }

        /** CHIP **/
        /**
         * @brief Get size of the chip
         * @return Size of the chip
         *
         * Defaults to the pixel grid size as given by \ref Detector::getGridSize.
         */
        virtual ROOT::Math::XYZVector getChipSize() const { return getGridSize(); }
        /**
         * @brief Get center of the chip in local coordinates
         * @return Center of the chip
         *
         * Defaults to the center of the grid as given by \ref Detector::getCenter() with sensor offset.
         */
        virtual ROOT::Math::XYZPoint getChipCenter() const {
            ROOT::Math::XYZVector offset(0, 0, -getSensorSize().z() / 2.0 - getChipSize().z() / 2.0);
            return getCenter() + offset;
        }

        /** PCB **/
        /**
         * @brief Get size of the PCB
         * @return Size of the PCB
         *
         * Defaults to the pixel grid size as given by \ref Detector::getGridSize.
         */
        virtual ROOT::Math::XYZVector getPCBSize() const { return getGridSize(); }
        /**
         * @brief Get center of the chip in local coordinates
         * @return Center of the chip
         *
         * Default to the center of the grid as given by \ref Detector::getCenter() with sensor and chip offset.
         */
        virtual ROOT::Math::XYZPoint getPCBCenter() const {
            ROOT::Math::XYZVector offset(0, 0, -getSensorSize().z() / 2.0 - getChipSize().z() - getPCBSize().z() / 2.0);
            return getCenter() + offset;
        }

    protected:
        std::string type_;

        double sensor_thickness_;
        ROOT::Math::XYVector pixel_size_;
    };
} // namespace allpix

#endif // ALLPIX_GEOMETRY_MANAGER_H
