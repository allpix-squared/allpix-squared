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

#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "core/config/ConfigReader.hpp"
#include "core/config/Configuration.hpp"
#include "core/geometry/DetectorAssembly.hpp"
#include "core/geometry/PixelDetectorModel.hpp"
#include "objects/Pixel.hpp"

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
    if(std::fabs(offset_) < std::numeric_limits<double>::epsilon()) {
        throw InvalidValueError(
            config, "pixel_offset", "for pixel offset of zero, the regular pixel geometry should be used");
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
    bool const odd_row = (static_cast<int>(std::lround(position.y() / pixel_size_.y())) % 2) != 0;

    auto pixel_x = static_cast<int>(std::lround(position.x() / pixel_size_.x() - (odd_row ? offset_ : 0.)));
    auto pixel_y = static_cast<int>(std::lround(position.y() / pixel_size_.y()));
    return {pixel_x, pixel_y};
}

std::set<Pixel::Index> StaggeredPixelDetectorModel::getNeighbors(const Pixel::Index& idx, const size_t distance) const {
    std::set<Pixel::Index> neighbors;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"

    // Double-resolution integer coordinates for the center
    const int cx = 2 * idx.x() + ((idx.y() % 2 != 0) ? (offset_ > 0 ? 1 : -1) : 0);
    // Squared distance threshold
    const int r2 = (2 * static_cast<int>(distance) + 1) * (2 * static_cast<int>(distance) + 1);

    // Bounding box is distance in all directions, plus 1 due to diagonal reach
    for(int ny = idx.y() - static_cast<int>(distance); ny <= idx.y() + static_cast<int>(distance); ++ny) {
        for(int nx = idx.x() - static_cast<int>(distance); nx <= idx.x() + static_cast<int>(distance); ++nx) {

            // Convert neighbor x to double-resolution center x
            auto ncx = 2 * nx;
            if(ny % 2 != 0) {
                ncx += (offset_ > 0) ? 1 : -1;
            }

            // Squared distance in scaled space
            auto dx2 = ncx - cx;
            auto dy2 = ny - idx.y();

            // Check distance to central pixel, add + 1 to distance to include diagonal elements
            if((dx2 * dx2 + dy2 * dy2 * 4) <= r2) {
                if(!PixelDetectorModel::isWithinMatrix(nx, ny)) {
                    continue;
                }
                neighbors.insert({nx, ny});
            }
        }
    }
#pragma GCC diagnostic pop

    return neighbors;
}

bool StaggeredPixelDetectorModel::areNeighbors(const Pixel::Index& seed,
                                               const Pixel::Index& entrant,
                                               const size_t distance) const {
    // Double-resolution x positions
    const int x1d = 2 * seed.x() + ((seed.y() % 2 != 0) ? (offset_ > 0 ? 1 : -1) : 0);
    const int x2d = 2 * entrant.x() + ((entrant.y() % 2 != 0) ? (offset_ > 0 ? 1 : -1) : 0);

    const int dx = x2d - x1d;
    const int dy = entrant.y() - seed.y();

    // Double-resolution squared distance
    const int dist2 = dx * dx + 4 * dy * dy;
    // Squared distance threshold
    const int r2 = (2 * static_cast<int>(distance) + 1) * (2 * static_cast<int>(distance) + 1);

    return dist2 <= r2;
}
