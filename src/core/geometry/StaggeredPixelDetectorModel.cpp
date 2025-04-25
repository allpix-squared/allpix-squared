/**
 * @file
 * @brief Implementation of a staggered pixel detector model
 *
 * @copyright Copyright (c) 2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "StaggeredPixelDetectorModel.hpp"
#include "core/module/exceptions.h"

using namespace allpix;

StaggeredPixelDetectorModel::StaggeredPixelDetectorModel(std::string type,
                                                         const std::shared_ptr<DetectorAssembly>& assembly,
                                                         const ConfigReader& reader,
                                                         const Configuration& config)
    : PixelDetectorModel(std::move(type), assembly, reader, config) {

    // Read tile offset - for now only possible along x, applied to odd rows
    offset_ = config.get<double>("pixel_offset");
    if(std::fabs(offset_) >= 1.0) {
        throw InvalidValueError(
            config,
            "pixel_offset",
            "pixel offset should be provided in fractions of the pitch and cannot be larger than or equal to +-1.0");
    }
}

ROOT::Math::XYZPoint StaggeredPixelDetectorModel::getMatrixCenter() const {
    // The matrix center is calculated relative to the local origin. It is shifted by the pixel offset along x only if the
    // offset is negative, because then the origin of the local coordinate system is not the leftmost pixel anymore.
    return {getMatrixSize().x() / 2.0 - getPixelSize().x() / 2.0 + (offset_ < 0 ? offset_ : 0.) * getPixelSize().x(),
            getMatrixSize().y() / 2.0 - getPixelSize().y() / 2.0,
            0};
}

ROOT::Math::XYZVector StaggeredPixelDetectorModel::getMatrixSize() const {
    // Matrix size is extended in x by the pixel offset:
    return {(getNPixels().x() + std::fabs(offset_)) * getPixelSize().x(), getNPixels().y() * getPixelSize().y(), 0};
}

bool StaggeredPixelDetectorModel::isWithinMatrix(const ROOT::Math::XYZPoint& position) const {
    // Get indices of this pixel:
    const auto indices = getPixelIndex(position);
    // Check if indices are valid:
    return PixelDetectorModel::isWithinMatrix(indices.first, indices.second);
}

ROOT::Math::XYZPoint StaggeredPixelDetectorModel::getPixelCenter(const int x, const int y) const {
    auto size = getPixelSize();
    auto local_x = size.x() * (x + ((y % 2) != 0 ? offset_ : 0.));
    auto local_y = size.y() * y;
    return {local_x, local_y, 0};
}

std::pair<int, int> StaggeredPixelDetectorModel::getPixelIndex(const ROOT::Math::XYZPoint& position) const {
    // Check if we have an odd or even row
    bool odd_row = (static_cast<int>(std::lround(position.y() / pixel_size_.y())) % 2) != 0;

    auto pixel_x = static_cast<int>(std::lround(position.x() / pixel_size_.x() - (odd_row ? offset_ : 0.)));
    auto pixel_y = static_cast<int>(std::lround(position.y() / pixel_size_.y()));
    return {pixel_x, pixel_y};
}

std::set<Pixel::Index> StaggeredPixelDetectorModel::getNeighbors(const Pixel::Index& idx, const size_t distance) const {
    std::set<Pixel::Index> neighbors;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
    // Neighbor rows are the same as in a regular aligned pixel model
    for(int y = idx.y() - static_cast<int>(distance); y <= idx.y() + static_cast<int>(distance); y++) {
        // In neighbor columns, we have three neighbors in odd rows and only two in even rows
        // For positive offsets, the leftmost fall away
        int distance_left = static_cast<int>(distance) - (y % 2 == 0 && offset_ > 0 ? 1 : 0);
        // For negative offsets, the rightmost fall away
        int distance_right = static_cast<int>(distance) - (y % 2 == 0 && offset_ < 0 ? 1 : 0);

        for(int x = idx.x() - distance_left; x <= idx.x() + distance_right; x++) {
            if(!PixelDetectorModel::isWithinMatrix(x, y)) {
                continue;
            }
            neighbors.insert({x, y});
        }
    }
#pragma GCC diagnostic pop

    return neighbors;
}

bool StaggeredPixelDetectorModel::areNeighbors(const Pixel::Index& seed,
                                               const Pixel::Index& entrant,
                                               const size_t distance) const {
    // Along y, it's just adjacent rows
    bool neighbor_in_y = static_cast<size_t>(std::abs(seed.y() - entrant.y())) <= distance;

    // Along x, we need to take the offset into account
    // For positive offsets, the leftmost fall away in even rows
    int distance_left = static_cast<int>(distance) - (entrant.y() % 2 == 0 && offset_ > 0 ? 1 : 0);
    // For negative offsets, the rightmost fall away in even rows
    int distance_right = static_cast<int>(distance) - (entrant.y() % 2 == 0 && offset_ < 0 ? 1 : 0);

    bool neighbor_in_x = (seed.x() > entrant.x() && (seed.x() - entrant.x()) <= distance_right) ||
                         (seed.x() < entrant.x() && (entrant.x() - seed.x()) <= distance_left);

    return (neighbor_in_x && neighbor_in_y);
}
