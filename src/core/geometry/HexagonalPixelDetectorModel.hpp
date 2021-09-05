/**
 * @file
 * @brief Detector model with hexagonal pixel shape
 *
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_HEXAGONAL_PIXEL_DETECTOR_H
#define ALLPIX_HEXAGONAL_PIXEL_DETECTOR_H

#include "DetectorModel.hpp"

namespace allpix {
    /**
     * @ingroup DetectorModels
     * @brief Detector model with hexagonal pixel grid
     *
     * The implementation of this detector model follows the axial coordinate system approach where two non-orthogonal axes
     * along the rows and (slanted) columns of the hexagonal grid are defined. An excellent description of this coordinate
     * systam along with all necessary math and transformations can be found at https://www.redblobgames.com/grids/hexagons
     */
    class HexagonalPixelDetectorModel : public DetectorModel {
    public:
        /**
         * @brief constructor of a hexagonal pixel detector model
         * @param  type   Name of the model type
         * @param  reader Configuration reader with description of the model
         */
        explicit HexagonalPixelDetectorModel(std::string type, const ConfigReader& reader)
            : DetectorModel(std::move(type), reader) {
            auto config = reader.getHeaderConfiguration();

            // Select shape orientation
            pixel_type_ = config.get<Pixel::Type>("pixel_type");
            if(pixel_type_ != Pixel::Type::HEXAGON_FLAT && pixel_type_ != Pixel::Type::HEXAGON_POINTY) {
                throw InvalidValueError(
                    config,
                    "pixel_type",
                    "for this model, only pixel types 'hexagon_pointy' and 'hexagon_flat' are available");
            }
        }

        /**
         * @brief Essential virtual destructor
         */
        ~HexagonalPixelDetectorModel() override = default;

        /**
         * @brief Get local coordinate of the position and rotation center in global frame
         * @note It can be a bit counter intuitive that this is not usually the origin, neither the geometric center of the
         * model, but the geometric center of the sensitive part. This way, the position of the sensing element is invariant
         * under rotations
         *
         * The center coordinate corresponds to the \ref Detector::getPosition "position" in the global frame.
         */
        // FIXME implement correctly, this is rectangular pixels currently!
        ROOT::Math::XYZPoint getCenter() const override {
            return {
                getGridSize().x() / 2.0 - getPixelSize().x() / 2.0, getGridSize().y() / 2.0 - getPixelSize().y() / 2.0, 0};
        }

        /**
         * @brief Returns a pixel center in local coordinates
         * @param x X- (or column-) coordinate of the pixel
         * @param y Y- (or row-) coordinate of the pixel
         * @return Coordinates of the pixel center
         */
        ROOT::Math::XYZPoint getPixelCenter(const int x, const int y) const override {
            const std::array<double, 4> transform_pointy{std::sqrt(3.0), std::sqrt(3.0) / 2.0, 0.0, 3.0 / 2.0};
            const std::array<double, 4> transform_flat{3.0 / 2.0, 0.0, std::sqrt(3.0) / 2.0, std::sqrt(3.0)};

            auto local_z = getSensorCenter().z() - getSensorSize().z() / 2.0;

            if(pixel_type_ == Pixel::Type::HEXAGON_POINTY) {
                return {(transform_pointy.at(0) * x + transform_pointy.at(1) * y) * pixel_size_.x(),
                        (transform_pointy.at(2) * x + transform_pointy.at(3) * y) * pixel_size_.y(),
                        local_z};
            } else {
                return {(transform_flat.at(0) * x + transform_flat.at(1) * y) * pixel_size_.x(),
                        (transform_flat.at(2) * x + transform_flat.at(3) * y) * pixel_size_.y(),
                        local_z};
            }
        };

        /**
         * @brief Return X,Y indices of a pixel corresponding to a local position in a sensor.
         * @param position Position in local coordinates of the detector model
         * @return X,Y pixel indices
         *
         * @note No checks are performed on whether these indices represent an existing pixel or are within the pixel matrix.
         */
        std::pair<int, int> getPixelIndex(const ROOT::Math::XYZPoint& position) const override {
            const std::array<double, 4> inv_transform_pointy{std::sqrt(3.0) / 3.0, -1.0 / 3.0, 0.0, 2.0 / 3.0};
            const std::array<double, 4> inv_transform_flat{2.0 / 3.0, 0.0, -1.0 / 3.0, std::sqrt(3.0) / 3.0};

            auto pt = ROOT::Math::XYPoint(position.x() / pixel_size_.x(), position.y() / pixel_size_.y());
            double q = 0, r = 0;
            if(pixel_type_ == Pixel::Type::HEXAGON_POINTY) {
                q = inv_transform_pointy.at(0) * pt.x() + inv_transform_pointy.at(1) * pt.y();
                r = inv_transform_pointy.at(2) * pt.x() + inv_transform_pointy.at(3) * pt.y();
            } else {
                q = inv_transform_flat.at(0) * pt.x() + inv_transform_flat.at(1) * pt.y();
                r = inv_transform_flat.at(2) * pt.x() + inv_transform_flat.at(3) * pt.y();
            }
            return round_to_nearest_hex(q, r);
        };

        /**
         * @brief Returns if a set of pixel coordinates is within the grid of pixels defined for the device
         * @param x X- (or column-) coordinate to be checked
         * @param y Y- (or row-) coordinate to be checked
         * @return True if pixel coordinates are within the pixel grid, false otherwise
         *
         * In an axial-coordinates hexagon grid, simply checking for x and y to be between 0 and number_of_pixels will create
         * a rhombus which does lack the upper-left pixels and which has surplus pixels at the upper-right corner. We
         * therefore need to check the allowed range along x as a function of the y coordinate. The integer division by two
         * ensures we allow for one more x coordinate every other row in y.
         */
        bool isWithinPixelGrid(const int x, const int y) const override {
            // FIXME check if this depends on pointy vs. flat!
            return !(y < 0 || y >= static_cast<int>(number_of_pixels_.y()) || x < 0 - y / 2 ||
                     x >= static_cast<int>(number_of_pixels_.x()) - y / 2);
        };

        /**
         * @brief Returns if a pixel index is within the grid of pixels defined for the device
         * @param pixel_index Pixel index to be checked
         * @return True if pixel_index is within the pixel grid, false otherwise
         */
        bool isWithinPixelGrid(const Pixel::Index& pixel_index) const override {
            return isWithinPixelGrid(pixel_index.x(), pixel_index.y());
        };

        /**
         * @brief Return gridsize along X,Y of a hexagonal sensor grid.
         * @return X and Y gridlength length in mm
         */
        ROOT::Math::XYZVector getGridSize() const override {
            // FIXME re-do math!
            if(pixel_type_ == Pixel::Type::HEXAGON_POINTY) {
                LOG(ERROR) << "grid size: " << Units::display(number_of_pixels_.x() * 2 * pixel_size_.x(), "um") << ", "
                           << Units::display(number_of_pixels_.y() * 2 * pixel_size_.y(), "um");
                return {number_of_pixels_.x() * 2 * pixel_size_.x(), number_of_pixels_.y() * 2 * pixel_size_.y(), 0};
            } else {
                return {number_of_pixels_.x() * 2 * pixel_size_.x(), number_of_pixels_.y() * 2 * pixel_size_.y(), 0};
            }
        }

        /**
         * @brief Return a set containing all pixels neighboring the given one with a configurable maximum distance
         * @param idx       Index of the pixel in question
         * @param distance  Distance for pixels to be considered neighbors
         * @return Set of neighboring pixel indices, including the initial pixel
         *
         * @note The returned set should always also include the initial pixel indices the neighbors are calculated for
         */
        std::set<Pixel::Index> getNeighbors(const Pixel::Index& idx, const size_t distance) const override {
            std::set<Pixel::Index> neighbors;

            for(int x = idx.x() - static_cast<int>(distance); x <= idx.x() + static_cast<int>(distance); x++) {
                for(int y = idx.y() - static_cast<int>(distance); y <= idx.y() + static_cast<int>(distance); y++) {
                    // "cut off" the corners of the rectangle around the index in question to make it a hexagon, remove
                    // indices outside the pixel grid
                    if(std::abs(x - idx.x() + y - idx.y()) <= static_cast<int>(distance) && isWithinPixelGrid(x, y)) {
                        neighbors.insert({x, y});
                    }
                }
            }
            return neighbors;
        }

        /**
         * @brief Return a set containing all pixels neighboring the two given pixels with a configurable maximum distance
         * @param idx       Index of the first pixel in question
         * @param last_idx  Index of the second pixel in question
         * @param distance  Distance for pixels to be considered neighbors
         * @return Set of neighboring pixel indices, including the two initial pixels
         *
         * @note The returned set should always also include the initial pixel indices the neighbors are calculated for
         */
        std::set<Pixel::Index>
        getNeighbors(const Pixel::Index& idx, const Pixel::Index& last_idx, const size_t distance) const override {
            std::set<Pixel::Index> neighbors;

            auto x_lower = std::min(idx.x(), last_idx.x()) - static_cast<int>(distance);
            auto x_higher = std::max(idx.x(), last_idx.x()) + static_cast<int>(distance);
            auto y_lower = std::min(idx.y(), last_idx.y()) - static_cast<int>(distance);
            auto y_higher = std::max(idx.y(), last_idx.y()) + static_cast<int>(distance);

            for(int x = x_lower; x <= x_higher; x++) {
                for(int y = y_lower; y <= y_higher; y++) {
                    // Remove indices outside the pixel grid
                    if(isWithinPixelGrid(x, y)) {
                        // "cut off" the corners of the rectangle around the indices in question to make it a "prolonged"
                        // hexagon
                        if(std::abs(x - idx.x() + y - idx.y()) <= static_cast<int>(distance) ||
                           std::abs(x - last_idx.x() + y - last_idx.y()) <= static_cast<int>(distance)) {
                            neighbors.insert({x, y});
                        }
                    }
                }
            }
            return neighbors;
        };

        /**
         * @brief Check if two pixel indices are neighbors to each other
         * @param  seed    Initial pixel index
         * @param  entrant Entrant pixel index to be tested
         * @param distance  Distance for pixels to be considered neighbors
         * @return         Boolean whether pixels are neighbors or not
         */
        bool areNeighbors(const Pixel::Index& seed, const Pixel::Index& entrant, const size_t distance) const override {
            return (hex_distance(seed.x(), seed.y(), entrant.x(), entrant.y()) <= distance);
        };

    private:
        // Rounding is more easy in cubic coordinates, so we need to reconstruct the third coordinate from the other two as
        // z = - x - y:
        std::pair<int, int> round_to_nearest_hex(double x, double y) const {
            auto q = static_cast<int>(std::round(x));
            auto r = static_cast<int>(std::round(y));
            auto s = static_cast<int>(std::round(-x - y));
            double q_diff = std::abs(q - x);
            double r_diff = std::abs(r - y);
            double s_diff = std::abs(s - (-x - y));
            if(q_diff > r_diff and q_diff > s_diff) {
                q = -r - s;
            } else if(r_diff > s_diff) {
                r = -q - s;
            }
            return {q, r};
        };

        // The distance between two hexagons in cubic coordinates is half the Manhattan distance. To use axial coordinates,
        // we have to reconstruct the third coordinate z = - x - y:
        size_t hex_distance(double x1, double x2, double y1, double y2) const {
            return static_cast<size_t>((std::abs(x1 - x2) + std::abs(y1 - y2) + std::abs(-x1 - y1 + x2 + y2)) / 2);
        };
    }; // End of HexagonalPixelDetectorModel class
} // namespace allpix
#endif /* ALLPIX_HEXAGONAL_PIXEL_DETECTOR_H */
