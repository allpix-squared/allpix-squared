/**
 * @file
 * @brief Base of detector models
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

/**
 * @defgroup DetectorModels Detector models
 * @brief Collection of detector models supported by the framework
 */

#ifndef ALLPIX_DETECTOR_MODEL_H
#define ALLPIX_DETECTOR_MODEL_H

#include <array>
#include <string>
#include <utility>

#include <Math/Point2D.h>
#include <Math/Point3D.h>
#include <Math/RotationZ.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

#include "core/config/ConfigReader.hpp"
#include "core/config/exceptions.h"
#include "core/utils/log.h"
#include "objects/Pixel.hpp"
#include "tools/ROOT.h"

#include "DetectorAssembly.hpp"
#include "SupportLayer.hpp"

namespace allpix {
    /**
     * @brief Sensor materials
     */
    enum class SensorMaterial {
        SILICON = 1,            ///< Silicon
        GALLIUM_ARSENIDE,       ///< Gallium Arsenide
        GERMANIUM,              ///< Germanium
        CADMIUM_TELLURIDE,      ///< Cadmium Telluride
        CADMIUM_ZINC_TELLURIDE, ///< Cadmium Zinc Telluride
        DIAMOND,                ///< Diamond
        SILICON_CARBIDE,        ///< Silicon Carbide
        GALLIUM_NITRIDE,        ///< Gallium Nitride
        CESIUM_LEAD_BROMIDE,    ///< CsPbBr3 (Perovskite)
    };

    /**
     * @brief Type of dopant
     */
    enum class Dopant {
        PHOSPHORUS = 0,
        ARSENIC,
    };

    /**
     * @ingroup DetectorModels
     * @brief Base of all detector models
     *
     * Implements the minimum required for a detector model. A model always has a pixel grid with a specific pixel size. The
     * pixel grid defines the base size of the sensor, chip and support. Excess length can be specified. Every part of the
     * detector model has a defined center and size which can be overloaded by specialized detector models. The basic
     * detector model also defines the rotation center in the local coordinate system.
     */
    class DetectorModel {
    public:
        /**
         * @brief Factory to dynamically create detector model objects
         * @param name Name of the model
         * @param reader Reader with the configuration for this model
         * @return Detector model instantiated from the configuration
         */
        static std::shared_ptr<DetectorModel> factory(const std::string& name, const ConfigReader&);

        /**
         * @brief Helper method to determine if this detector model is of a given type
         * The template parameter needs to be specified specifically, i.e.
         *     if(model->is<PixelDetectorModel>()) { }
         * @return Boolean indication whether this model is of the given type or not
         */
        template <class T> bool is() { return dynamic_cast<T*>(this) != nullptr; }

        /**
         * @brief Helper class to hold implant definitions for a detector model
         */
        class Implant {
            friend class DetectorModel;

        public:
            enum class Type { FRONTSIDE, BACKSIDE };
            enum class Shape { RECTANGLE, ELLIPSE };

            /**
             * @brief Get the offset of the implant with respect to the pixel center
             * @return Implant offset
             */
            ROOT::Math::XYZVector getOffset() const { return offset_; }
            /**
             * @brief Get the implant orientation as rotation around its z-axis
             * @return Implant orientation
             */
            ROOT::Math::RotationZ getOrientation() const { return orientation_; }
            /**
             * @brief Get the size of the implant
             * @return Size of the implant
             */
            ROOT::Math::XYZVector getSize() const { return size_; }
            /**
             * @brief Return the type of the implant
             * @return implant type
             */
            Type getType() const { return type_; }

            /**
             * @brief Return the shape of the implant
             * @return implant shape
             */
            Shape getShape() const { return shape_; }

            /**
             * @brief helper to calculate containment of points with this implant
             * @param  position Position relative to the pixel center
             * @return True if point lies within implant, false otherwise
             */
            bool contains(const ROOT::Math::XYZVector& position) const;

            /**
             * @brief Fetch the configuration of this implant
             * @return Implant configuration
             */
            const Configuration& getConfiguration() const { return config_; }

            /**
             * @brief calculate intersection of line segment with implant. The first intersection in the given direction is
             * returned.
             * @throws std::invalid_argument if intersection calculation is not implemented for the implant type
             * @param  direction Direction vector of line
             * @param  position  Position vector of line
             * @return Closest intersection point with implant, std::nullopt if none could be found
             */
            std::optional<ROOT::Math::XYZPoint> intersect(const ROOT::Math::XYZVector& direction,
                                                          const ROOT::Math::XYZPoint& position) const;

        private:
            /**
             * @brief Constructs an implant, used in \ref DetectorModel::addImplant
             * @param type Type of the implant
             * @param shape Shape of the implant cross-section
             * @param size Size of the implant
             * @param offset Offset of the implant from the pixel center
             * @param orientation Rotation angle around the implant z-axis
             * @param config Configuration
             */
            Implant(Type type,
                    Shape shape,
                    ROOT::Math::XYZVector size,
                    ROOT::Math::XYZVector offset,
                    ROOT::Math::RotationZ orientation,
                    Configuration config)
                : type_(type), shape_(shape), size_(std::move(size)), offset_(std::move(offset)), orientation_(orientation),
                  config_(std::move(config)) {}

            // Actual parameters returned
            Type type_;
            Shape shape_;
            ROOT::Math::XYZVector size_;
            ROOT::Math::XYZVector offset_;
            ROOT::Math::RotationZ orientation_;
            Configuration config_;
        };

        /**
         * @brief Constructs the base detector model
         * @param type Name of the model type
         * @param assembly Detector assembly object with information about ASIC and packaging
         * @param reader Configuration reader with description of the model
         * @param config Configuration holding the empty section of the configuration file
         */
        explicit DetectorModel(std::string type,
                               std::shared_ptr<DetectorAssembly> assembly,
                               const ConfigReader& reader,
                               const Configuration& config);

        /**
         * @brief Essential virtual destructor
         */
        virtual ~DetectorModel() = default;

        /**
         * @brief Get the configuration associated with this model
         * @return Configuration used to construct the model
         */
        std::vector<Configuration> getConfigurations() const;

        /**
         * @brief Get the type of the model
         * @return Model type
         */
        const std::string& getType() const { return type_; }

        const std::shared_ptr<DetectorAssembly> getAssembly() const { return assembly_; }

        /**
         * @brief Get local coordinate of the position and rotation center in global frame
         * @note It can be a bit counter intuitive that this is not usually the origin, neither the geometric center of the
         * model, but the geometric center of the sensitive part. This way, the position of the sensing element is invariant
         * under rotations
         *
         * The center coordinate corresponds to the \ref Detector::getPosition "position" in the global frame.
         */
        virtual ROOT::Math::XYZPoint getMatrixCenter() const {
            return {getMatrixSize().x() / 2.0 - getPixelSize().x() / 2.0,
                    getMatrixSize().y() / 2.0 - getPixelSize().y() / 2.0,
                    0};
        }

        /**
         * @brief Get local coordinate of the geometric center of the model
         * @note This returns the center of the geometry model, i.e. including all support layers, passive readout chips et
         * cetera.
         */
        virtual ROOT::Math::XYZPoint getModelCenter() const;

        /**
         * @brief Get size of the wrapper box around the model that contains all elements
         * @return Size of the detector model
         *
         * All elements of the model are covered by a box centered around \ref DetectorModel::getModelCenter. This
         * means that the extend of the model should be calculated using the geometrical center as reference, not the
         * position returned by \ref DetectorModel::getMatrixCenter.
         */
        virtual ROOT::Math::XYZVector getSize() const;

        /* PIXEL GRID */
        /**
         * @brief Get number of pixels (replicated blocks in generic sensors)
         * @return Number of two dimensional pixels
         */
        ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<unsigned int>> getNPixels() const {
            return number_of_pixels_;
        }

        /**
         * @brief Get size of a single pixel
         * @return Size of a pixel
         */
        const ROOT::Math::XYVector& getPixelSize() const { return pixel_size_; }
        /**
         * @brief Get type of the pixels
         * @return TYpe of the pixels indicating their shape
         */
        Pixel::Type getPixelType() const { return pixel_type_; }

        /**
         * @brief Return all implants
         * @return List of all the implants
         */
        inline const std::vector<Implant>& getImplants() const { return implants_; };

        /**
         * @brief Get total size of the pixel grid
         * @return Size of the pixel grid
         *
         * @warning The grid has zero thickness
         * @note This is basically a 2D method, but provided in 3D because it is primarily used there
         */
        virtual ROOT::Math::XYZVector getMatrixSize() const {
            return {getNPixels().x() * getPixelSize().x(), getNPixels().y() * getPixelSize().y(), 0};
        }

        /* SENSOR */
        /**
         * @brief Get size of the sensor
         * @return Size of the sensor
         *
         * Calculated from \ref DetectorModel::getMatrixSize "pixel grid size", sensor excess and sensor thickness
         */
        virtual ROOT::Math::XYZVector getSensorSize() const {
            ROOT::Math::XYZVector excess_thickness((sensor_excess_.at(1) + sensor_excess_.at(3)),
                                                   (sensor_excess_.at(0) + sensor_excess_.at(2)),
                                                   sensor_thickness_);
            return getMatrixSize() + excess_thickness;
        }
        /**
         * @brief Get center of the sensor in local coordinates
         * @return Center of the sensor
         *
         * Center of the sensor with excess taken into account
         */
        virtual ROOT::Math::XYZPoint getSensorCenter() const {
            ROOT::Math::XYZVector offset(
                (sensor_excess_.at(1) - sensor_excess_.at(3)) / 2.0, (sensor_excess_.at(0) - sensor_excess_.at(2)) / 2.0, 0);
            return getMatrixCenter() + offset;
        }
        /**
         * @brief Get the material of the sensor
         * @return Material of the sensor
         */
        SensorMaterial getSensorMaterial() const { return sensor_material_; }

        /* CHIP */
        /**
         * @brief Get size of the chip
         * @return Size of the chip
         *
         * Calculated from \ref DetectorModel::getMatrixSize "pixel grid size", sensor excess and chip thickness
         */
        virtual ROOT::Math::XYZVector getChipSize() const {
            ROOT::Math::XYZVector excess_thickness(
                assembly_->getChipExcess().x(), assembly_->getChipExcess().y(), assembly_->getChipThickness());
            return getMatrixSize() + excess_thickness;
        }
        /**
         * @brief Get center of the chip in local coordinates
         * @return Center of the chip
         *
         * Center of the chip calculated from chip excess and sensor offset
         */
        virtual ROOT::Math::XYZPoint getChipCenter() const {
            ROOT::Math::XYZVector offset(assembly_->getChipOffset().x() / 2.0,
                                         assembly_->getChipOffset().y() / 2.0,
                                         getSensorSize().z() / 2.0 + getChipSize().z() / 2.0 +
                                             assembly_->getChipOffset().z());
            return getMatrixCenter() + offset;
        }

        /* SUPPORT */
        /**
         * @brief Return all layers of support
         * @return List of all the support layers
         *
         * This method internally computes the correct center of all the supports by stacking them in linear order on both
         * the chip and the sensor side.
         */
        virtual std::vector<SupportLayer> getSupportLayers() const;

        /**
         * @brief Returns if a local position is within the sensitive device
         * @param local_pos Position in local coordinates of the detector model
         * @return True if a local position is within the sensor, false otherwise
         *
         * @note This method is purely virtual and must be implemented by the respective concrete detector model classes
         */
        virtual bool isWithinSensor(const ROOT::Math::XYZPoint& local_pos) const = 0;

        /**
         * @brief Returns if a local position is on the sensor boundary
         * @param local_pos Position in local coordinates of the detector model
         * @return True if a local position is on the sensor boundary, false otherwise
         *
         * @note This method is purely virtual and must be implemented by the respective concrete detector model classes
         */
        virtual bool isOnSensorBoundary(const ROOT::Math::XYZPoint& local_pos) const = 0;

        /**
         * @brief Calculate exit point of step outside sensor volume from one point inside the sensor (before step) and one
         * point outside (after step).
         * @throws std::invalid_argument if no intersection of track segment with sensor volume can be found
         * @param  inside Position before the step, inside the sensor volume
         * @param  outside  Position after the step, outside the sensor volume
         * @return Exit point of the sensor in local coordinates
         *
         * @note This method is purely virtual and must be implemented by the respective concrete detector model classes
         */
        virtual ROOT::Math::XYZPoint getSensorIntercept(const ROOT::Math::XYZPoint& inside,
                                                        const ROOT::Math::XYZPoint& outside) const = 0;

        /**
         * @brief Returns if a local position is within the pixel implant region of the sensitive device.
         *
         * If the implant is defined as 3D volume in the detector model, this method returns true if the given position is
         * found within the implant volume. If the impland is configured with two dimensions only, i.e. an area on the sensor
         * surface, the additional depth parameter is used to create a volume within which carriers are considered inside the
         * implant.
         *
         * @param local_pos Position in local coordinates of the detector model
         * @return Either the implant in which the position is located, or false
         */
        virtual std::optional<Implant> isWithinImplant(const ROOT::Math::XYZPoint& local_pos) const;

        /**
         * @brief Calculate entry point of step into impant volume from one point outside the implant (before step) and one
         * point inside (after step).
         * @throws std::invalid_argument if no intersection of track segment with implant volume can be found
         * @param implant The implant the intercept should be calculated for
         * @param  outside Position before the step, outside the implant volume
         * @param  inside  Position after the step, inside the implant volume
         * @return Entry point in implant in local coordinates of the sensor
         */
        ROOT::Math::XYZPoint getImplantIntercept(const Implant& implant,
                                                 const ROOT::Math::XYZPoint& outside,
                                                 const ROOT::Math::XYZPoint& inside) const;

        /**
         * @brief Returns if a pixel index is within the grid of pixels defined for the device
         * @param pixel_index Pixel index to be checked
         * @return True if pixel_index is within the pixel grid, false otherwise
         *
         * @note This method is purely virtual and must be implemented by the respective concrete detector model classes
         */
        virtual bool isWithinMatrix(const Pixel::Index& pixel_index) const = 0;

        /**
         * @brief Returns if a set of pixel coordinates is within the grid of pixels defined for the device
         * @param x X- (or column-) coordinate to be checked
         * @param y Y- (or row-) coordinate to be checked
         * @return True if pixel coordinates are within the pixel grid, false otherwise
         *
         * @note This method is purely virtual and must be implemented by the respective concrete detector model classes
         */
        virtual bool isWithinMatrix(const int x, const int y) const = 0;

        /**
         * @brief Returns if a position is within the grid of pixels defined for the device
         * @param position Position in local coordinates of the detector model
         * @return True if position within the pixel grid, false otherwise
         *
         * @note This method is virtual and can be implemented by detector models for faster calculations
         */
        virtual bool isWithinMatrix(const ROOT::Math::XYZPoint& position) const {
            auto [index_x, index_y] = getPixelIndex(position);
            return isWithinMatrix(index_x, index_y);
        }

        /**
         * @brief Returns a pixel center in local coordinates
         * @param x X- (or column-) coordinate of the pixel
         * @param y Y- (or row-) coordinate of the pixel
         * @return Coordinates of the pixel center
         *
         * @note This method is purely virtual and must be implemented by the respective concrete detector model classes
         */
        virtual ROOT::Math::XYZPoint getPixelCenter(const int x, const int y) const = 0;
        /**
         * @brief Return X,Y indices of a pixel corresponding to a local position in a sensor.
         * @param local_pos Position in local coordinates of the detector model
         * @return X,Y pixel indices
         *
         * @note No checks are performed on whether these indices represent an existing pixel or are within the pixel matrix.
         *
         * @note This method is purely virtual and must be implemented by the respective concrete detector model classes
         */
        virtual std::pair<int, int> getPixelIndex(const ROOT::Math::XYZPoint& local_pos) const = 0;

        /**
         * @brief Return a set containing all pixels of the matrix
         * @return Set of all pixel indices of the matrix
         *
         * @note This method is purely virtual and must be implemented by the respective concrete detector model classes
         */
        virtual std::set<Pixel::Index> getPixels() const = 0;

        /**
         * @brief Return a set containing all pixels neighboring the given one with a configurable maximum distance
         * @param idx       Index of the pixel in question
         * @param distance  Distance for pixels to be considered neighbors
         * @return Set of neighboring pixel indices, including the initial pixel
         *
         * @note The returned set should always also include the initial pixel indices the neighbors are calculated for
         *
         * @note This method is purely virtual and must be implemented by the respective concrete detector model classes
         */
        virtual std::set<Pixel::Index> getNeighbors(const Pixel::Index& idx, const size_t distance) const = 0;

        /**
         * @brief Check if two pixel indices are neighbors to each other
         * @param  seed    Initial pixel index
         * @param  entrant Entrant pixel index to be tested
         * @param distance  Distance for pixels to be considered neighbors
         * @return         Boolean whether pixels are neighbors or not
         *
         * @note This method is purely virtual and must be implemented by the respective concrete detector model classes
         */
        virtual bool areNeighbors(const Pixel::Index& seed, const Pixel::Index& entrant, const size_t distance) const = 0;

    protected:
        /**
         * @brief Set number of pixels (replicated blocks in generic sensors)
         * @param val Number of two dimensional pixels
         */
        void setNPixels(ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<unsigned int>> val) {
            number_of_pixels_ = std::move(val);
        }

        /**
         * @brief Set the size of a pixel
         * @param val Size of a pixel
         */
        void setPixelSize(ROOT::Math::XYVector val) { pixel_size_ = std::move(val); }

        /**
         * @brief Set the thickness of the sensor
         * @param val Thickness of the sensor
         */
        void setSensorThickness(double val) { sensor_thickness_ = val; }
        /**
         * @brief Set the excess at the top of the sensor (positive y-coordinate)
         * @param val Sensor top excess
         */
        void setSensorExcessTop(double val) { sensor_excess_.at(0) = val; }
        /**
         * @brief Set the excess at the right of the sensor (positive x-coordinate)
         * @param val Sensor right excess
         */
        void setSensorExcessRight(double val) { sensor_excess_.at(1) = val; }
        /**
         * @brief Set the excess at the bottom of the sensor (negative y-coordinate)
         * @param val Sensor bottom excess
         */
        void setSensorExcessBottom(double val) { sensor_excess_.at(2) = val; }
        /**
         * @brief Set the excess at the left of the sensor (negative x-coordinate)
         * @param val Sensor right excess
         */
        void setSensorExcessLeft(double val) { sensor_excess_.at(3) = val; }

        std::string type_;

        ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<unsigned int>> number_of_pixels_;
        ROOT::Math::XYVector pixel_size_;
        Pixel::Type pixel_type_{Pixel::Type::RECTANGLE};

        double sensor_thickness_{};
        std::array<double, 4> sensor_excess_{};
        SensorMaterial sensor_material_{SensorMaterial::SILICON};

        std::shared_ptr<DetectorAssembly> assembly_;
        std::vector<Implant> implants_;
        std::vector<SupportLayer> support_layers_;

    private:
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
         * @brief Add a new implant
         * @param type Type of the implant
         * @param shape Shape of the implant cross-section
         * @param size Size of the implant
         * @param offset Offset of the implant from the pixel center
         * @param orientation Rotation angle around the implant z-axis
         * @param config Configuration of the implant
         */
        void addImplant(const Implant::Type& type,
                        const Implant::Shape& shape,
                        ROOT::Math::XYZVector size,
                        const ROOT::Math::XYVector& offset,
                        double orientation,
                        const Configuration& config);

        /**
         * @brief Add a new layer of support
         * @param size Size of the support in the x,y-plane
         * @param thickness Thickness of the support
         * @param offset Offset of the support in the x,y-plane
         * @param material Material of the support
         * @param type Type of the hole
         * @param location Location of the support (either 'sensor' or 'chip')
         * @param hole_size Size of the optional hole in the support
         * @param hole_offset Offset of the hole from its default position
         */
        void addSupportLayer(const ROOT::Math::XYVector& size,
                             double thickness,
                             ROOT::Math::XYZVector offset,
                             std::string material,
                             std::string type,
                             const SupportLayer::Location location,
                             const ROOT::Math::XYVector& hole_size,
                             ROOT::Math::XYVector hole_offset) {
            ROOT::Math::XYZVector full_size(size.x(), size.y(), thickness);
            ROOT::Math::XYZVector full_hole_size(hole_size.x(), hole_size.y(), thickness);
            support_layers_.push_back(SupportLayer(std::move(full_size),
                                                   std::move(offset),
                                                   std::move(material),
                                                   std::move(type),
                                                   location,
                                                   std::move(full_hole_size),
                                                   std::move(hole_offset)));
        }

        // Validation of the detector model
        virtual void validate();

        ConfigReader reader_;
    };
} // namespace allpix

#endif // ALLPIX_DETECTOR_MODEL_H
