/**
 * @file
 * @brief Implementation of hexagonal pixel detector model
 *
 * @copyright Copyright (c) 2021-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "HexagonalPixelDetectorModel.hpp"
#include "core/module/exceptions.h"

using namespace allpix;

HexagonalPixelDetectorModel::HexagonalPixelDetectorModel(std::string type,
                                                         const std::shared_ptr<DetectorAssembly>& assembly,
                                                         const ConfigReader& reader,
                                                         Configuration& header_config)
    : PixelDetectorModel(std::move(type), assembly, reader, header_config) {

    // Select shape orientation
    pixel_type_ = header_config.get<Pixel::Type>("pixel_type");
    if(pixel_type_ != Pixel::Type::HEXAGON_FLAT && pixel_type_ != Pixel::Type::HEXAGON_POINTY) {
        throw InvalidValueError(header_config,
                                "pixel_type",
                                "for this model, only pixel types 'hexagon_pointy' and 'hexagon_flat' are available");
    }
}

ROOT::Math::XYZPoint HexagonalPixelDetectorModel::getMatrixCenter() const {
    auto grid = getMatrixSize();
    auto corner_offset_left = pixel_size_.x() / 2 * std::cos(M_PI * (start_angle() + 3) / 3);   // corner 3
    auto corner_offset_bottom = pixel_size_.y() / 2 * std::sin(M_PI * (start_angle() + 4) / 3); // corner 4
    return {grid.x() / 2.0 + corner_offset_left, grid.y() / 2.0 + corner_offset_bottom, 0};
}

double HexagonalPixelDetectorModel::get_pixel_center_x(const int x, const int y) const {
    if(pixel_type_ == Pixel::Type::HEXAGON_POINTY) {
        return (transform_pointy_.at(0) * x + transform_pointy_.at(1) * y) * pixel_size_.x() / 2;
    } else {
        return (transform_flat_.at(0) * x + transform_flat_.at(1) * y) * pixel_size_.x() / 2;
    }
}

double HexagonalPixelDetectorModel::get_pixel_center_y(const int x, const int y) const {
    if(pixel_type_ == Pixel::Type::HEXAGON_POINTY) {
        return (transform_pointy_.at(2) * x + transform_pointy_.at(3) * y) * pixel_size_.y() / 2;
    } else {
        return (transform_flat_.at(2) * x + transform_flat_.at(3) * y) * pixel_size_.y() / 2;
    }
}

ROOT::Math::XYZPoint HexagonalPixelDetectorModel::getPixelCenter(const int x, const int y) const {
    return {get_pixel_center_x(x, y), get_pixel_center_y(x, y), 0};
}

std::pair<int, int> HexagonalPixelDetectorModel::getPixelIndex(const ROOT::Math::XYZPoint& position) const {
    auto pt = ROOT::Math::XYPoint(position.x() / pixel_size_.x() * 2, position.y() / pixel_size_.y() * 2);
    double q = 0, r = 0;
    if(pixel_type_ == Pixel::Type::HEXAGON_POINTY) {
        q = inv_transform_pointy_.at(0) * pt.x() + inv_transform_pointy_.at(1) * pt.y();
        r = inv_transform_pointy_.at(2) * pt.x() + inv_transform_pointy_.at(3) * pt.y();
    } else {
        q = inv_transform_flat_.at(0) * pt.x() + inv_transform_flat_.at(1) * pt.y();
        r = inv_transform_flat_.at(2) * pt.x() + inv_transform_flat_.at(3) * pt.y();
    }
    return round_to_nearest_hex(q, r);
}

/*
 * In an axial-coordinates hexagon grid, simply checking for x and y to be between 0 and number_of_pixels will create
 * a rhombus which does lack the upper-left pixels and which has surplus pixels at the upper-right corner. We
 * therefore need to check the allowed range along x as a function of the y coordinate. The integer division by two
 * ensures we allow for one more x coordinate every other row in y.
 */
bool HexagonalPixelDetectorModel::isWithinMatrix(const int x, const int y) const {
    // Check the valid pixel indices - this depends on the orientation of the axial index coordinate system with respect to
    // the cartesian local coordinate system, so we need to allow different indices depending on the hexagon orientation:
    if(pixel_type_ == Pixel::Type::HEXAGON_POINTY) {
        return !(y < 0 || y >= static_cast<int>(number_of_pixels_.y()) || x < 0 - y / 2 ||
                 x >= static_cast<int>(number_of_pixels_.x()) - y / 2);
    } else {
        return !(x < 0 || x >= static_cast<int>(number_of_pixels_.x()) || y < 0 - x / 2 ||
                 y >= static_cast<int>(number_of_pixels_.y()) - x / 2);
    }
}

bool HexagonalPixelDetectorModel::isWithinMatrix(const Pixel::Index& pixel_index) const {
    return isWithinMatrix(pixel_index.x(), pixel_index.y());
}

ROOT::Math::XYZVector HexagonalPixelDetectorModel::getMatrixSize() const {
    auto corner_offset_right = pixel_size_.x() / 2 * std::cos(M_PI * start_angle() / 3);        // corner 0
    auto corner_offset_top = pixel_size_.y() / 2 * std::sin(M_PI * (start_angle() + 1) / 3);    // corner 1
    auto corner_offset_left = pixel_size_.x() / 2 * std::cos(M_PI * (start_angle() + 3) / 3);   // corner 3
    auto corner_offset_bottom = pixel_size_.y() / 2 * std::sin(M_PI * (start_angle() + 4) / 3); // corner 4

    // Top and right boundaries:
    auto limit_top = corner_offset_top +
                     get_pixel_center_y((number_of_pixels_.y() > 1 ? 1 : 0), static_cast<int>(number_of_pixels_.y()) - 1);
    auto limit_right = corner_offset_right +
                       get_pixel_center_x(static_cast<int>(number_of_pixels_.x()) - 1, (number_of_pixels_.y() > 1 ? 1 : 0));

    return {limit_right - corner_offset_left, limit_top - corner_offset_bottom, 0};
}

std::set<Pixel::Index> HexagonalPixelDetectorModel::getNeighbors(const Pixel::Index& idx, const size_t distance) const {
    std::set<Pixel::Index> neighbors;

    for(int x = idx.x() - static_cast<int>(distance); x <= idx.x() + static_cast<int>(distance); x++) {
        for(int y = idx.y() - static_cast<int>(distance); y <= idx.y() + static_cast<int>(distance); y++) {
            if(hex_distance(idx.x(), idx.y(), x, y) <= distance && isWithinMatrix(x, y)) {
                neighbors.insert({x, y});
            }
        }
    }
    return neighbors;
}

bool HexagonalPixelDetectorModel::areNeighbors(const Pixel::Index& seed,
                                               const Pixel::Index& entrant,
                                               const size_t distance) const {
    return (hex_distance(seed.x(), seed.y(), entrant.x(), entrant.y()) <= distance);
}

// Rounding is more easy in cubic coordinates, so we need to reconstruct the third coordinate from the other two as z = - x -
// y:
std::pair<int, int> HexagonalPixelDetectorModel::round_to_nearest_hex(double x, double y) const {
    auto q = static_cast<int>(std::lround(x));
    auto r = static_cast<int>(std::lround(y));
    auto s = static_cast<int>(std::lround(-x - y));
    double q_diff = std::abs(q - x);
    double r_diff = std::abs(r - y);
    double s_diff = std::abs(s - (-x - y));
    if(q_diff > r_diff and q_diff > s_diff) {
        q = -r - s;
    } else if(r_diff > s_diff) {
        r = -q - s;
    }
    return {q, r};
}

// The distance between two hexagons in cubic coordinates is half the Manhattan distance. To use axial coordinates, we have
// to reconstruct the third coordinate z = - x - y:
size_t HexagonalPixelDetectorModel::hex_distance(double x1, double y1, double x2, double y2) const {
    return static_cast<size_t>(std::abs(x1 - x2) + std::abs(y1 - y2) + std::abs(-x1 - y1 + x2 + y2)) / 2;
}
