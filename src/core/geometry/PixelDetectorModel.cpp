/**
 * @file
 * @brief Implementation of a pixel detector model
 *
 * @copyright Copyright (c) 2021-2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "PixelDetectorModel.hpp"
#include "core/module/exceptions.h"
#include "tools/liang_barsky.h"

#include <Math/Translation3D.h>

using namespace allpix;

PixelDetectorModel::PixelDetectorModel(std::string type,
                                       const std::shared_ptr<DetectorAssembly>& assembly,
                                       const ConfigReader& reader)
    : DetectorModel(std::move(type), assembly, reader) {
    using namespace ROOT::Math;
    auto config = reader.getHeaderConfiguration();

    // Number of pixels
    setNPixels(config.get<DisplacementVector2D<Cartesian2D<unsigned int>>>("number_of_pixels"));
    // Size of the pixels
    auto pixel_size = config.get<XYVector>("pixel_size");
    setPixelSize(pixel_size);

    // Size of the collection diode implant on each pixels, defaults to the full pixel size when not specified
    XYZVector implant_size;
    try {
        // Attempt to read a three-dimensional implant definition:
        implant_size = config.get<XYZVector>("implant_size");
    } catch(ConfigurationError&) {
        // If 3D fails or key is not set at all, attempt to read a (flat) 2D implant definition, defaulting to full pixel
        auto implant_area = config.get<XYVector>("implant_size", pixel_size);
        implant_size = XYZVector(implant_area.x(), implant_area.y(), 0);
    }
    if(implant_size.x() > pixel_size.x() || implant_size.y() > pixel_size.y()) {
        throw InvalidValueError(config, "implant_size", "implant size cannot be larger than pixel pitch");
    }
    if(implant_size.z() > getSensorSize().z()) {
        throw InvalidValueError(config, "implant_size", "implant depth cannot be larger than sensor thickness");
    }
    setImplantSize(implant_size);
    setImplantMaterial(config.get<std::string>("implant_material", "aluminum"));

    // Offset of the collection diode implant from the pixel center, defaults to zero.
    auto implant_offset = config.get<XYVector>("implant_offset", {0, 0});
    if(std::fabs(implant_offset.x()) + implant_size.x() / 2 > pixel_size.x() / 2 ||
       std::fabs(implant_offset.y()) + implant_size.y() / 2 > pixel_size.y() / 2) {
        throw InvalidValueError(config, "implant_offset", "implant exceeds pixel cell. Reduce implant size or offset");
    }
    setImplantOffset(implant_offset);
}

/**
 * The definition of inside the sensor is determined by the detector model
 */
bool PixelDetectorModel::isWithinSensor(const ROOT::Math::XYZPoint& local_pos) const {
    auto sensor_center = getSensorCenter();
    auto sensor_size = getSensorSize();
    return (2 * std::fabs(local_pos.z() - sensor_center.z()) <= sensor_size.z()) &&
           (2 * std::fabs(local_pos.y() - sensor_center.y()) <= sensor_size.y()) &&
           (2 * std::fabs(local_pos.x() - sensor_center.x()) <= sensor_size.x());
}

/**
 * The definition of inside the implant region is determined by the detector model
 *
 * @note The pixel implant currently is always positioned symmetrically, in the center of the pixel cell.
 */
bool PixelDetectorModel::isWithinImplant(const ROOT::Math::XYZPoint& local_pos) const {

    auto [xpixel, ypixel] = getPixelIndex(local_pos);
    auto inPixelPos = local_pos - getPixelCenter(xpixel, ypixel);

    return (std::fabs(inPixelPos.x()) <= std::fabs(getImplantSize().x() / 2) &&
            std::fabs(inPixelPos.y()) <= std::fabs(getImplantSize().y() / 2) &&
            local_pos.z() >= getSensorSize().z() / 2 - getImplantSize().z());
}

/**
 * The definition of the pixel grid size is determined by the detector model
 */
bool PixelDetectorModel::isWithinMatrix(const Pixel::Index& pixel_index) const {
    return !(pixel_index.x() < 0 || pixel_index.x() >= static_cast<int>(number_of_pixels_.x()) || pixel_index.y() < 0 ||
             pixel_index.y() >= static_cast<int>(number_of_pixels_.y()));
}

/**
 * The definition of the pixel grid size is determined by the detector model
 */
bool PixelDetectorModel::isWithinMatrix(const int x, const int y) const {
    return !(x < 0 || x >= static_cast<int>(number_of_pixels_.x()) || y < 0 || y >= static_cast<int>(number_of_pixels_.y()));
}

ROOT::Math::XYZPoint PixelDetectorModel::getPixelCenter(const int x, const int y) const {
    auto size = getPixelSize();
    auto local_x = size.x() * x;
    auto local_y = size.y() * y;
    return {local_x, local_y, 0};
}

std::pair<int, int> PixelDetectorModel::getPixelIndex(const ROOT::Math::XYZPoint& position) const {
    auto pixel_x = static_cast<int>(std::lround(position.x() / pixel_size_.x()));
    auto pixel_y = static_cast<int>(std::lround(position.y() / pixel_size_.y()));
    return {pixel_x, pixel_y};
}

std::set<Pixel::Index> PixelDetectorModel::getNeighbors(const Pixel::Index& idx, const size_t distance) const {
    std::set<Pixel::Index> neighbors;

    for(int x = idx.x() - static_cast<int>(distance); x <= idx.x() + static_cast<int>(distance); x++) {
        for(int y = idx.y() - static_cast<int>(distance); y <= idx.y() + static_cast<int>(distance); y++) {
            if(!isWithinMatrix(x, y)) {
                continue;
            }
            neighbors.insert({x, y});
        }
    }

    return neighbors;
}

bool PixelDetectorModel::areNeighbors(const Pixel::Index& seed, const Pixel::Index& entrant, const size_t distance) const {
    return (static_cast<size_t>(std::abs(seed.x() - entrant.x())) <= distance &&
            static_cast<size_t>(std::abs(seed.y() - entrant.y())) <= distance);
}

ROOT::Math::XYZPoint PixelDetectorModel::getSensorIntercept(const ROOT::Math::XYZPoint& inside,
                                                            const ROOT::Math::XYZPoint& outside) const {
    // Get direction vector of motion *out of* sensor
    auto direction = (outside - inside).Unit();
    // We have to be centered around the sensor box. This means we need to shift by the matrix center
    auto translation_local = ROOT::Math::Translation3D(static_cast<ROOT::Math::XYZVector>(getMatrixCenter()));

    auto intersection_point =
        LiangBarsky::closestIntersection(direction, translation_local.Inverse()(inside), getSensorSize());

    // Get intersection from Liang-Barsky line clipping and re-transform to local coordinates:
    if(intersection_point) {
        return translation_local(intersection_point.value());
    } else {
        return inside;
    }
}
