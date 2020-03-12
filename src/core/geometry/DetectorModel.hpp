/**
 * @file
 * @brief Base of detector models
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
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
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

#include "core/config/ConfigReader.hpp"
#include "core/config/exceptions.h"
#include "core/utils/log.h"
#include "tools/ROOT.h"

namespace allpix {
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
         * @brief Helper class to hold support layers for a detector model
         */
        class SupportLayer {
            friend class DetectorModel;
            // FIXME Friending this class is broken
            friend class HybridPixelDetectorModel;

        public:
            /**
             * @brief Get the center of the support layer
             * @return Center of the support layer
             */
            ROOT::Math::XYZPoint getCenter() const { return center_; }
            /**
             * @brief Get the size of the support layer
             * @return Size of the support layer
             */
            ROOT::Math::XYZVector getSize() const { return size_; }
            /**
             * @brief Get the material of the support layer
             * @return Support material
             */
            std::string getMaterial() const { return material_; }
            /**
             * @brief Return if the support layer contains a hole
             * @return True if the support layer has a hole, false otherwise
             */
            bool hasHole() { return hole_size_.x() > 1e-9 && hole_size_.y() > 1e-9; }
            /**
             * @brief Get the center of the hole in the support layer
             * @return Center of the hole
             */
            ROOT::Math::XYZPoint getHoleCenter() const {
                return center_ + ROOT::Math::XYZVector(hole_offset_.x(), hole_offset_.y(), 0);
            }
            /**
             * @brief Get the full size of the hole in the support layer
             * @return Size of the hole
             */
            ROOT::Math::XYZVector getHoleSize() const { return hole_size_; }
            /**
             * @brief Get the location of the support layer
             */
            std::string getLocation() const { return location_; }

        private:
            /**
             * @brief Constructs a support layer, used in \ref DetectorModel::addSupportLayer
             * @param size Size of the support layer
             * @param offset Offset of the support layer from the center
             * @param material Material of the support layer
             * @param location Location of the support material
             * @param hole_size Size of an optional hole (zero vector if no hole)
             * @param hole_offset Offset of the optional hole from the center of the support layer
             */
            SupportLayer(ROOT::Math::XYZVector size,
                         ROOT::Math::XYZVector offset,
                         std::string material,
                         std::string location,
                         ROOT::Math::XYZVector hole_size,
                         ROOT::Math::XYVector hole_offset)
                : size_(std::move(size)), material_(std::move(material)), hole_size_(std::move(hole_size)),
                  offset_(std::move(offset)), hole_offset_(std::move(hole_offset)), location_(std::move(location)) {}

            // Actual parameters returned
            ROOT::Math::XYZPoint center_;
            ROOT::Math::XYZVector size_;
            std::string material_;
            ROOT::Math::XYZVector hole_size_;

            // Internal parameters to calculate return parameters
            ROOT::Math::XYZVector offset_;
            ROOT::Math::XYVector hole_offset_;
            std::string location_;
        };

        /**
         * @brief Constructs the base detector model
         * @param type Name of the model type
         * @param reader Configuration reader with description of the model
         */
        explicit DetectorModel(std::string type, ConfigReader reader);

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
         * @brief Get the configuration associated with this model
         * @return Configuration used to construct the model
         */
        std::vector<Configuration> getConfigurations() const;

        /**
         * @brief Get the type of the model
         * @return Model type
         */
        std::string getType() const { return type_; }

        /**
         * @brief Get local coordinate of the position and rotation center in global frame
         * @note It can be a bit counter intuitive that this is not usually the origin, neither the geometric center of the
         * model, but the geometric center of the sensitive part. This way, the position of the sensing element is invariant
         * under rotations
         *
         * The center coordinate corresponds to the \ref Detector::getPosition "position" in the global frame.
         */
        virtual ROOT::Math::XYZPoint getCenter() const {
            return {
                getGridSize().x() / 2.0 - getPixelSize().x() / 2.0, getGridSize().y() / 2.0 - getPixelSize().y() / 2.0, 0};
        }

        /**
         * @brief Get local coordinate of the geometric center of the model
         * @note This returns the center of the geometry model, i.e. including all support layers, passive readout chips et
         * cetera.
         */
        virtual ROOT::Math::XYZPoint getGeometricalCenter() const {
            // Get the distance, on the z axis, between the sensor center and the detector (box) geometrical center
            double detector_thickness = getSize().z();
            double min_support_center =
                -getSensorSize().Z() / 2; // center (on the z axis) of the support (sensor side) with smallest z coordinate
            double min_support_size = 0.; // z size of the support (sensor side) with smallest z coordinate
            for(auto& support_layer : getSupportLayers()) {
                double centers_zDistance = (support_layer.getCenter()).z() - getSensorCenter().z();
                if(centers_zDistance < min_support_center) { // selects supports on the sensor side only
                    min_support_center = centers_zDistance;
                    min_support_size = (support_layer.getSize()).z();
                }
            }
            double support_sensorSide_thickness =
                std::abs(min_support_center) + min_support_size / 2 -
                getSensorSize().Z() / 2; // total thickness of support layers (including empty spaces) on the sensor side
            double zDistanceToGeoCenter = detector_thickness / 2 - support_sensorSide_thickness - getSensorSize().Z() / 2;

            return ROOT::Math::XYZPoint(getCenter().x(), getCenter().y(), zDistanceToGeoCenter);
        }

        /**
         * @brief Get size of the wrapper box around the model that contains all elements
         * @return Size of the detector model
         *
         * All elements of the model are covered by a box centered around \ref DetectorModel::getGeometricalCenter. This
         * means that the extend of the model should be calculated using the geometrical center as reference, not the positon
         * returned by \ref DetectorModel::getCenter.
         */
        virtual ROOT::Math::XYZVector getSize() const;

        /* PIXEL GRID */
        /**
         * @brief Get number of pixel (replicated blocks in generic sensors)
         * @return Number of two dimensional pixels
         */
        virtual ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<unsigned int>> getNPixels() const {
            return number_of_pixels_;
        }
        /**
         * @brief Set number of pixels (replicated blocks in generic sensors)
         * @param val Number of two dimensional pixels
         */
        void setNPixels(ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<unsigned int>> val) {
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
         * @brief Get size of the collection diode
         * @return Size of the collection diode implant
         */
        virtual ROOT::Math::XYVector getImplantSize() const { return implant_size_; }
        /**
         * @brief Set the size of the implant (collection diode) within a pixel
         * @param val Size of the collection diode implant
         */
        void setImplantSize(ROOT::Math::XYVector val) { implant_size_ = std::move(val); }
        /**
         * @brief Get total size of the pixel grid
         * @return Size of the pixel grid
         *
         * @warning The grid has zero thickness
         * @note This is basically a 2D method, but provided in 3D because it is primarily used there
         */
        ROOT::Math::XYZVector getGridSize() const {
            return {getNPixels().x() * getPixelSize().x(), getNPixels().y() * getPixelSize().y(), 0};
        }

        /* SENSOR */
        /**
         * @brief Get size of the sensor
         * @return Size of the sensor
         *
         * Calculated from \ref DetectorModel::getGridSize "pixel grid size", sensor excess and sensor thickness
         */
        virtual ROOT::Math::XYZVector getSensorSize() const {
            ROOT::Math::XYZVector excess_thickness((sensor_excess_.at(1) + sensor_excess_.at(3)),
                                                   (sensor_excess_.at(0) + sensor_excess_.at(2)),
                                                   sensor_thickness_);
            return getGridSize() + excess_thickness;
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
            return getCenter() + offset;
        }
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

        /* CHIP */
        /**
         * @brief Get size of the chip
         * @return Size of the chip
         *
         * Calculated from \ref DetectorModel::getGridSize "pixel grid size", sensor excess and chip thickness
         */
        virtual ROOT::Math::XYZVector getChipSize() const {
            ROOT::Math::XYZVector excess_thickness((sensor_excess_.at(1) + sensor_excess_.at(3)),
                                                   (sensor_excess_.at(0) + sensor_excess_.at(2)),
                                                   chip_thickness_);
            return getGridSize() + excess_thickness;
        }
        /**
         * @brief Get center of the chip in local coordinates
         * @return Center of the chip
         *
         * Center of the chip calculcated from chip excess and sensor offset
         */
        virtual ROOT::Math::XYZPoint getChipCenter() const {
            ROOT::Math::XYZVector offset((sensor_excess_.at(1) - sensor_excess_.at(3)) / 2.0,
                                         (sensor_excess_.at(0) - sensor_excess_.at(2)) / 2.0,
                                         getSensorSize().z() / 2.0 + getChipSize().z() / 2.0);
            return getCenter() + offset;
        }
        /**
         * @brief Set the thickness of the sensor
         * @param val Thickness of the sensor
         */
        void setChipThickness(double val) { chip_thickness_ = val; }

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
         * @brief Add a new layer of support
         * @param size Size of the support in the x,y-plane
         * @param thickness Thickness of the support
         * @param offset Offset of the support in the x,y-plane
         * @param material Material of the support
         * @param location Location of the support (either 'sensor' or 'chip')
         * @param hole_size Size of the optional hole in the support
         * @param hole_offset Offset of the hole from its default position
         */
        // FIXME: Location (and material) should probably be an enum instead
        void addSupportLayer(const ROOT::Math::XYVector& size,
                             double thickness,
                             ROOT::Math::XYZVector offset,
                             std::string material,
                             std::string location,
                             const ROOT::Math::XYVector& hole_size,
                             ROOT::Math::XYVector hole_offset) {
            ROOT::Math::XYZVector full_size(size.x(), size.y(), thickness);
            ROOT::Math::XYZVector full_hole_size(hole_size.x(), hole_size.y(), thickness);
            support_layers_.push_back(SupportLayer(full_size,
                                                   std::move(offset),
                                                   std::move(material),
                                                   std::move(location),
                                                   full_hole_size,
                                                   std::move(hole_offset)));
        }

    protected:
        std::string type_;

        ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<unsigned int>> number_of_pixels_;
        ROOT::Math::XYVector pixel_size_;
        ROOT::Math::XYVector implant_size_;

        double sensor_thickness_{};
        std::array<double, 4> sensor_excess_{};

        double chip_thickness_{};

        std::vector<SupportLayer> support_layers_;

    private:
        ConfigReader reader_;
    };
} // namespace allpix

#endif // ALLPIX_DETECTOR_MODEL_H
