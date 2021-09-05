/**
 * @file
 * @brief Implementation of hexagonal pixel detector model
 *
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "HexagonalPixelDetectorModel.hpp"
#include "core/module/exceptions.h"

using namespace allpix;

HexagonalPixelDetectorModel::HexagonalPixelDetectorModel(std::string type, const ConfigReader& reader)
    : DetectorModel(std::move(type), reader) {
    auto config = reader.getHeaderConfiguration();

    // Select shape orientation
    pixel_type_ = config.get<Pixel::Type>("pixel_type");
    if(pixel_type_ != Pixel::Type::HEXAGON_FLAT && pixel_type_ != Pixel::Type::HEXAGON_POINTY) {
        throw InvalidValueError(
            config, "pixel_type", "for this model, only pixel types 'hexagon_pointy' and 'hexagon_flat' are available");
    }
}

ROOT::Math::XYZPoint HexagonalPixelDetectorModel::getCenter() const {
    // FIXME implement correctly, this is rectangular pixels currently!
    return {getGridSize().x() / 2.0 - getPixelSize().x() / 2.0, getGridSize().y() / 2.0 - getPixelSize().y() / 2.0, 0};
}

ROOT::Math::XYZPoint HexagonalPixelDetectorModel::getPixelCenter(const int x, const int y) const {
    auto local_z = getSensorCenter().z() - getSensorSize().z() / 2.0;

    if(pixel_type_ == Pixel::Type::HEXAGON_POINTY) {
        return {(transform_pointy_.at(0) * x + transform_pointy_.at(1) * y) * pixel_size_.x(),
                (transform_pointy_.at(2) * x + transform_pointy_.at(3) * y) * pixel_size_.y(),
                local_z};
    } else {
        return {(transform_flat_.at(0) * x + transform_flat_.at(1) * y) * pixel_size_.x(),
                (transform_flat_.at(2) * x + transform_flat_.at(3) * y) * pixel_size_.y(),
                local_z};
    }
}

std::pair<int, int> HexagonalPixelDetectorModel::getPixelIndex(const ROOT::Math::XYZPoint& position) const {
    auto pt = ROOT::Math::XYPoint(position.x() / pixel_size_.x(), position.y() / pixel_size_.y());
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

bool HexagonalPixelDetectorModel::isWithinPixelGrid(const int x, const int y) const {
    // FIXME check if this depends on pointy vs. flat!
    return !(y < 0 || y >= static_cast<int>(number_of_pixels_.y()) || x < 0 - y / 2 ||
             x >= static_cast<int>(number_of_pixels_.x()) - y / 2);
}

bool HexagonalPixelDetectorModel::isWithinPixelGrid(const Pixel::Index& pixel_index) const {
    return isWithinPixelGrid(pixel_index.x(), pixel_index.y());
}

ROOT::Math::XYZVector HexagonalPixelDetectorModel::getGridSize() const {
    // FIXME re-do math!
    if(pixel_type_ == Pixel::Type::HEXAGON_POINTY) {
        LOG(ERROR) << "grid size: " << Units::display(number_of_pixels_.x() * 2 * pixel_size_.x(), "um") << ", "
                   << Units::display(number_of_pixels_.y() * 2 * pixel_size_.y(), "um");
        return {number_of_pixels_.x() * 2 * pixel_size_.x(), number_of_pixels_.y() * 2 * pixel_size_.y(), 0};
    } else {
        return {number_of_pixels_.x() * 2 * pixel_size_.x(), number_of_pixels_.y() * 2 * pixel_size_.y(), 0};
    }
}

std::set<Pixel::Index> HexagonalPixelDetectorModel::getNeighbors(const Pixel::Index& idx, const size_t distance) const {
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

std::set<Pixel::Index> HexagonalPixelDetectorModel::getNeighbors(const Pixel::Index& idx,
                                                                 const Pixel::Index& last_idx,
                                                                 const size_t distance) const {
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
}

bool HexagonalPixelDetectorModel::areNeighbors(const Pixel::Index& seed,
                                               const Pixel::Index& entrant,
                                               const size_t distance) const {
    return (hex_distance(seed.x(), seed.y(), entrant.x(), entrant.y()) <= distance);
}

// Rounding is more easy in cubic coordinates, so we need to reconstruct the third coordinate from the other two as z = - x -
// y:
std::pair<int, int> HexagonalPixelDetectorModel::round_to_nearest_hex(double x, double y) const {
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
}

// The distance between two hexagons in cubic coordinates is half the Manhattan distance. To use axial coordinates, we have
// to reconstruct the third coordinate z = - x - y:
size_t HexagonalPixelDetectorModel::hex_distance(double x1, double x2, double y1, double y2) const {
    return static_cast<size_t>((std::abs(x1 - x2) + std::abs(y1 - y2) + std::abs(-x1 - y1 + x2 + y2)) / 2);
}
