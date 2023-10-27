/**
 * @file
 * @brief Parameters of a radial strip detector model
 *
 * @copyright Copyright (c) 2022-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_RADIAL_STRIP_DETECTOR_MODEL_H
#define ALLPIX_RADIAL_STRIP_DETECTOR_MODEL_H

#include <numeric>
#include <string>
#include <utility>

#include <Math/Cartesian2D.h>
#include <Math/DisplacementVector2D.h>
#include <Math/Point3D.h>
#include <Math/Polar2D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>
#include <TMath.h>

#include "DetectorModel.hpp"

namespace allpix {

    /**
     * @ingroup DetectorModels
     * @brief Model of a radial strip detector. This is a model where the silicon
     * sensor is a trapezoid and the strips fan out radially from a focal point.
     */
    class RadialStripDetectorModel : public DetectorModel {
    public:
        /**
         * @brief Constructs the radial strip detector model
         * @param type Name of the model type
         * @param assembly Detector assembly object with information about ASIC and packaging
         * @param reader Configuration reader with description of the model
         * @param config Configuration reference holding the unnamed section of detector configuration
         */
        explicit RadialStripDetectorModel(std::string type,
                                          const std::shared_ptr<DetectorAssembly>& assembly,
                                          const ConfigReader& reader,
                                          Configuration& config);

        /**
         * @brief Get the number of strips in a given strip row
         * @param row Strip row (y-coordinate)
         * @return Number of strips in a given strip row
         */
        unsigned int getNumberOfStrips(unsigned int row) const { return number_of_strips_.at(row); }

        /**
         * @brief Get the strip length in a given strip row
         * @param row Strip row (y-coordinate)
         * @return Strip length in a given strip row
         */
        double getStripLength(unsigned int row) const { return strip_length_.at(row); }

        /**
         * @brief Get the angular strip pitch in a given strip row
         * @param row Strip row (y-coordinate)
         * @return Angular strip pitch in a given strip row
         */
        double getAngularPitch(unsigned int row) const { return angular_pitch_.at(row); }

        /**
         * @brief Get the maximum angular strip pitch in all strip rows
         * @return Maximum angular strip pitch in all strip rows
         */
        double getAngularPitchMax() const { return *std::max_element(angular_pitch_.begin(), angular_pitch_.end()); }

        /**
         * @brief Get the inner strip pitch in a given strip row
         * @param row Strip row (y-coordinate)
         * @return Inner strip pitch in a given strip row
         */
        double getInnerPitch(unsigned int row) const { return inner_pitch_.at(row); }

        /**
         * @brief Get the smaller base length of the trapezoidal sensor wrapper
         * @return Length of the smaller base of the trapezoidal sensor wrapper
         */
        double getSensorBaseInner() const { return sensor_base_.at(0); }

        /**
         * @brief Get the larger base length of the trapezoidal sensor wrapper
         * @return Length of the larger base of the trapezoidal sensor wrapper
         */
        double getSensorBaseOuter() const { return sensor_base_.at(1); }

        /**
         * @brief Get the maximum angle subtended by the widest strip row
         * @return Angle subtended by the widest strip row
         */
        double getRowAngleMax() const { return *std::max_element(row_angle_.begin(), row_angle_.end()); }

        /**
         * @brief Get the inner radius of a given strip row
         * @param row Strip row (y-coordinate)
         * @return Inner radius of a given strip row
         *
         * @note For N rows the row_radius_ vector has N+1 values. The first element is the inner
         *  radius of the first row, the last element is the outer radius of the last row (virtually
         *  the inner radius of the nonexistent (N+1)-th row).
         */
        double getRowRadius(unsigned int row) const { return row_radius_.at(row); }

        /**
         * @brief Get the vector of strip rows inner radii
         * @return Vector of strip rows inner radii
         *
         * @note For N rows the row_radius_ vector has N+1 values. The first element is the inner
         *  radius of the first row, the last element is the outer radius of the last row (virtually
         *  the inner radius of the nonexistent (N+1)-th row).
         */
        const std::vector<double>& getRowRadii() const { return row_radius_; }

        /**
         * @brief Get the radius of the sensor center
         * @return Radius of the sensor center
         */
        double getCenterRadius() const { return (row_radius_.at(0) + row_radius_.at(number_of_pixels_.y())) / 2; }

        /**
         * @brief Get the sensor stereo angle
         * @return Sensor stereo angle
         */
        double getStereoAngle() const { return stereo_angle_; }

        /**
         * @brief Get local coordinate of the position and rotation center in global frame
         * @return Local coordinate of the position and rotation center in global frame
         *
         * @note For a radial sensor the matrix center is located in the center of the sensor, defined by the center radius
         */
        ROOT::Math::XYZPoint getMatrixCenter() const override { return {0, getCenterRadius(), 0}; }

        /**
         * @brief Get size of the rectangular wrapper box around the model that contains all elements
         * @return Size of the detector model
         *
         * @warning While the actual sensor wrapper is build as a trapezoid,
         *   the wrapper referred to by this function is rectangular
         * @note The wrapper box for radial strip sensors has the dimensions:
         *  - X: the larger trapezoid base
         *  - Y: length of the trapezoid
         *  - Z: sensor thickness
         */
        ROOT::Math::XYZVector getSize() const override { return {sensor_base_.at(1), sensor_length_, sensor_thickness_}; }

        /**
         * @brief Returns the dimensions of a strip in a given row
         * @param row Strip row (y-coordinate)
         * @return Dimensions of a strip in a given row
         *
         * @note For radial strip models the pixel size is the size of the smallest rectangle circumscribed around the
         * trapezoidal strip in a given row
         */
        ROOT::Math::XYVector getStripSize(unsigned int row) const {
            return {inner_pitch_.at(row) + 2 * strip_length_.at(row) * tan(angular_pitch_.at(row) / 2),
                    strip_length_.at(row)};
        }
        /**
         * @brief Returns if a local position is within the sensitive device
         * @param position Position in local coordinates of the detector model
         * @return True if a local position is within the sensor, false otherwise
         */
        bool isWithinSensor(const ROOT::Math::XYZPoint& position) const override;

        /**
         * @brief Calculate exit point of step outside sensor volume from one point inside the sensor (before step) and one
         * point outside (after step).
         * @throws std::invalid_argument if no intersection of track segment with sensor volume can be found
         * @param  inside Position before the step, inside the sensor volume
         * @param  outside  Position after the step, outside the sensor volume
         * @return Exit point of the sensor in local coordinates
         *
         * @note This method currently only interpolates the z coordinate between the last two points and returns the last
         * position inside otherwise
         */
        ROOT::Math::XYZPoint getSensorIntercept(const ROOT::Math::XYZPoint& inside,
                                                const ROOT::Math::XYZPoint& outside) const override;

        /**
         * @brief Returns if a strip index is within the grid of strips defined for the device
         * @param strip_index Strip index to be checked
         * @return True if strip_index is within the strip grid, false otherwise
         */
        bool isWithinMatrix(const Pixel::Index& strip_index) const override;

        /**
         * @brief Returns if a set of strip coordinates is within the grid of strips defined for the device
         * @param x X- (or column-) coordinate to be checked
         * @param y Y- (or row-) coordinate to be checked
         * @return True if strip coordinates are within the strip grid, false otherwise
         */
        bool isWithinMatrix(const int x, const int y) const override {
            return !(y < 0 || y >= static_cast<int>(getNPixels().y()) || x < 0 ||
                     x >= static_cast<int>(number_of_strips_.at(static_cast<unsigned int>(y))));
        }

        /**
         * @brief Converts the local position in cartesian coordinates to polar coordinates
         * @param local_pos Position in local cartesian coordinates of the detector model
         * @return Local position in polar coordinates
         *
         * @note The polar coordinates are defined in a system where:
         *  - R is measured from the local coordinate center
         *  - Phi is measured from the strip focal point
         */
        ROOT::Math::Polar2DPoint getPositionPolar(const ROOT::Math::XYZPoint& local_pos) const;

        /**
         * @brief Converts the position in polar coordinates to cartesian coordinates in the local frame.
         * @param polar_pos Position in local polar coordinates of the detector model
         * @return Local position in cartesian coordinates
         */
        ROOT::Math::XYPoint getPositionCartesian(const ROOT::Math::Polar2DPoint& polar_pos) const;

        /**
         * @brief Returns a pixel center in local coordinates
         * @param x X- (or column-) coordinate of the pixel
         * @param y Y- (or row-) coordinate of the pixel
         * @return Coordinates of the pixel center
         */
        ROOT::Math::XYZPoint getPixelCenter(int x, int y) const override;

        /**
         * @brief Return X,Y indices of a pixel corresponding to a local position in a sensor.
         * @param position Position in local coordinates of the detector model
         * @return X,Y pixel indices
         *
         * @note No checks are performed on whether these indices represent an existing pixel or are within the pixel matrix.
         */
        std::pair<int, int> getPixelIndex(const ROOT::Math::XYZPoint& position) const override;

        /**
         * @brief Return a set containing all pixels neighboring the given one with a configurable maximum distance
         * @param idx       Index of the pixel in question
         * @param distance  Distance for pixels to be considered neighbors
         * @return Set of neighboring pixel indices, including the initial pixel
         *
         * @note The returned set should always also include the initial pixel indices the neighbors are calculated for
         */
        std::set<Pixel::Index> getNeighbors(const Pixel::Index& idx, const size_t distance) const override;

        /**
         * @brief Check if two pixel indices are neighbors to each other
         * @param seed     Initial pixel index
         * @param entrant  Entrant pixel index to be tested
         * @param distance Distance for pixels to be considered neighbors
         * @return         Boolean whether pixels are neighbors or not
         */
        bool areNeighbors(const Pixel::Index& seed, const Pixel::Index& entrant, const size_t distance) const override;

    private:
        /**
         * @brief Set the number of strips
         * @param val Vector describing number of strips in each strip row
         */
        void setNumberOfStrips(std::vector<unsigned int> val) { number_of_strips_ = std::move(val); }

        /**
         * @brief Set the strip length
         * @param val Vector describing strip length in each strip row
         */
        void setStripLength(std::vector<double> val) { strip_length_ = std::move(val); }

        /**
         * @brief Set the angular strip pitch
         * @param val Vector describing angular strip pitch in each row
         */
        void setAngularPitch(std::vector<double> val) { angular_pitch_ = std::move(val); }

        /**
         * @brief Set the inner strip pitch
         * @param val Vector describing inner strip pitch in each strip row
         */
        void setInnerPitch(std::vector<double> val) { inner_pitch_ = std::move(val); }

        /**
         * @brief Set the length of the trapezoidal sensor wrapper
         * @param val Length of the trapezoidal sensor wrapper
         */
        void setSensorLength(double val) { sensor_length_ = val; }

        /**
         * @brief Set the smaller base length of the trapezoidal sensor wrapper
         * @param val Length of the smaller base of the trapezoidal sensor wrapper
         */
        void setSensorBaseInner(double val) { sensor_base_.at(0) = val; }

        /**
         * @brief Set the larger base length of the trapezoidal sensor wrapper
         * @param val Length of the larger base of the trapezoidal sensor wrapper
         */
        void setSensorBaseOuter(double val) { sensor_base_.at(1) = val; }

        /**
         * @brief Set the angle subtended by each strip row
         * @param val Vector describing angle subtended by each strip row
         */
        void setRowAngle(std::vector<double> val) { row_angle_ = std::move(val); }

        /**
         * @brief Set outer radius of each strip row
         * @param val Vector describing outer radius of each strip row
         */
        void setRowRadius(std::vector<double> val) { row_radius_ = std::move(val); }

        /**
         * @brief Set the sensor stereo angle
         * @param val Sensor stereo angle
         */
        void setStereoAngle(double val) { stereo_angle_ = val; }

        std::vector<unsigned int> number_of_strips_{};
        std::vector<double> strip_length_{};
        std::vector<double> angular_pitch_{};
        std::vector<double> inner_pitch_{};
        double stereo_angle_{};

        std::array<double, 2> sensor_base_{};
        double sensor_length_{};
        std::vector<double> row_radius_{};
        std::vector<double> row_angle_{};

        ROOT::Math::XYZVector focus_translation_;
    };
} // namespace allpix

#endif /* ALLPIX_RADIAL_STRIP_DETECTOR_MODEL_H */
