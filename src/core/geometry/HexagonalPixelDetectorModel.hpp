/**
 * @file
 * @brief Detector model with hexagonal pixel shape
 *
 * @copyright Copyright (c) 2021-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_HEXAGONAL_PIXEL_DETECTOR_H
#define ALLPIX_HEXAGONAL_PIXEL_DETECTOR_H

#include "PixelDetectorModel.hpp"

namespace allpix {
    /**
     * @ingroup DetectorModels
     * @brief Detector model with hexagonal pixel grid
     *
     * The implementation of this detector model follows the axial coordinate system approach where two non-orthogonal axes
     * along the rows and (slanted) columns of the hexagonal grid are defined. An excellent description of this coordinate
     * systam along with all necessary math and transformations can be found at https://www.redblobgames.com/grids/hexagons
     */
    class HexagonalPixelDetectorModel : public PixelDetectorModel {
    public:
        /**
         * @brief constructor of a hexagonal pixel detector model
         * @param type   Name of the model type
         * @param assembly Detector assembly object with information about ASIC and packaging
         * @param reader Configuration reader with description of the model
         */
        explicit HexagonalPixelDetectorModel(std::string type,
                                             const std::shared_ptr<DetectorAssembly>& assembly,
                                             const ConfigReader& reader,
                                             Configuration& header_config);

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
        ROOT::Math::XYZPoint getMatrixCenter() const override;

        /**
         * @brief Returns a pixel center in local coordinates
         * @param x X- (or column-) coordinate of the pixel
         * @param y Y- (or row-) coordinate of the pixel
         * @return Coordinates of the pixel center
         */
        ROOT::Math::XYZPoint getPixelCenter(const int x, const int y) const override;

        /**
         * @brief Return X,Y indices of a pixel corresponding to a local position in a sensor.
         * @param position Position in local coordinates of the detector model
         * @return X,Y pixel indices
         *
         * @note No checks are performed on whether these indices represent an existing pixel or are within the pixel matrix.
         */
        std::pair<int, int> getPixelIndex(const ROOT::Math::XYZPoint& position) const override;

        /**
         * @brief Returns if a set of pixel coordinates is within the grid of pixels defined for the device
         * @param x X- (or column-) coordinate to be checked
         * @param y Y- (or row-) coordinate to be checked
         * @return True if pixel coordinates are within the pixel grid, false otherwise
         */
        bool isWithinMatrix(const int x, const int y) const override;

        /**
         * @brief Returns if a pixel index is within the grid of pixels defined for the device
         * @param pixel_index Pixel index to be checked
         * @return True if pixel_index is within the pixel grid, false otherwise
         */
        bool isWithinMatrix(const Pixel::Index& pixel_index) const override;

        /**
         * @brief Return grid size along X,Y of a hexagonal sensor grid.
         * @return X and Y grid length in mm
         */
        ROOT::Math::XYZVector getMatrixSize() const override;

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
         * @param  seed    Initial pixel index
         * @param  entrant Entrant pixel index to be tested
         * @param distance  Distance for pixels to be considered neighbors
         * @return         Boolean whether pixels are neighbors or not
         */
        bool areNeighbors(const Pixel::Index& seed, const Pixel::Index& entrant, const size_t distance) const override;

    private:
        // Transformations from axial coordinates to cartesian coordinates
        const std::array<double, 4> transform_pointy_{std::sqrt(3.0), std::sqrt(3.0) / 2.0, 0.0, 3.0 / 2.0};
        const std::array<double, 4> transform_flat_{3.0 / 2.0, 0.0, std::sqrt(3.0) / 2.0, std::sqrt(3.0)};

        // Inverse transformations, going from cartesian coordinates to axial coordinates
        const std::array<double, 4> inv_transform_pointy_{std::sqrt(3.0) / 3.0, -1.0 / 3.0, 0.0, 2.0 / 3.0};
        const std::array<double, 4> inv_transform_flat_{2.0 / 3.0, 0.0, -1.0 / 3.0, std::sqrt(3.0) / 3.0};

        /**
         * @brief Helper to calculate the center along x of a hexagon in cartesian coordinates
         * @param  x Axial column index
         * @param  y Axial row index
         * @return Position of the pixel center along x in cartesian coordinates
         */
        double get_pixel_center_x(const int x, const int y) const;

        /**
         * @brief Helper to calculate the center along y of a hexagon in cartesian coordinates
         * @param  x Axial column index
         * @param  y Axial row index
         * @return Position of the pixel center along y in cartesian coordinates
         */
        double get_pixel_center_y(const int x, const int y) const;

        /**
         * @brief Helper to determine the starting angle for the position of the first corner
         * @return Starting angle
         */
        double start_angle() const { return (pixel_type_ == Pixel::Type::HEXAGON_POINTY ? 0.5 : 0.0); }

        /**
         * @brief Helper function to correctly round floating-point hexagonal positions to the nearest hexagon.
         * @param x  Column axial coordinate of the hexagon
         * @param y  Row axial coordinate of the hexagon
         * @return Indices of nearest hexagon
         */
        std::pair<int, int> round_to_nearest_hex(double x, double y) const;

        /**
         * @brief Helper function to calculate the distance between two hexagons using Manhattan metric in cubic coordinates
         * @param x1  Column axial coordinate of the first hexagon
         * @param y1  Row axial coordinate of the first hexagon
         * @param x2  Column axial coordinate of the second hexagon
         * @param y2  Row axial coordinate of the second hexagon
         * @return    Absolute distance between the hexagons
         */
        size_t hex_distance(double x1, double y1, double x2, double y2) const;
    };
} // namespace allpix

#endif /* ALLPIX_HEXAGONAL_PIXEL_DETECTOR_H */
