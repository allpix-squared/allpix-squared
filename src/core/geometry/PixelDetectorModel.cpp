/**
 * @file
 * @brief Implementation of a pixel detector model
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "PixelDetectorModel.hpp"
#include "core/module/exceptions.h"

using namespace allpix;

PixelDetectorModel::PixelDetectorModel(std::string type, ConfigReader reader) : DetectorModel(std::move(type), reader) {
    using namespace ROOT::Math;
    auto config = reader.getHeaderConfiguration();

    // Number of pixels
    setNPixels(config.get<DisplacementVector2D<Cartesian2D<unsigned int>>>("number_of_pixels"));
    // Size of the pixels
    auto pixel_size = config.get<XYVector>("pixel_size");
    setPixelSize(pixel_size);
    // Size of the collection diode implant on each pixel, defaults to the full pixel size when not specified
    auto implant_size = config.get<XYVector>("implant_size", pixel_size);
    if(implant_size.x() > pixel_size.x() || implant_size.y() > pixel_size.y()) {
        throw InvalidValueError(config, "implant_size", "implant size cannot be larger than pixel pitch");
    }
    setImplantSize(implant_size);
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
            std::fabs(inPixelPos.y()) <= std::fabs(getImplantSize().y() / 2));
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

ROOT::Math::XYZPoint DetectorModel::getPixelCenter(const int x, const int y) const {
    auto size = getPixelSize();
    auto local_x = size.x() * x;
    auto local_y = size.y() * y;
    auto local_z = getSensorCenter().z() - getSensorSize().z() / 2.0;

    return {local_x, local_y, local_z};
}

std::pair<int, int> PixelDetectorModel::getPixelIndex(const ROOT::Math::XYZPoint& position) const {
    auto pixel_x = static_cast<int>(std::round(position.x() / pixel_size_.x()));
    auto pixel_y = static_cast<int>(std::round(position.y() / pixel_size_.y()));
    return {pixel_x, pixel_y};
}

std::set<Pixel::Index> PixelDetectorModel::getNeighbors(const Pixel::Index& idx, const size_t distance) const {
    std::set<Pixel::Index> neighbors;

    for(int x = static_cast<int>(idx.x() - distance); x <= static_cast<int>(idx.x() + distance); x++) {
        for(int y = static_cast<int>(idx.y() - distance); y <= static_cast<int>(idx.y() + distance); y++) {
            if(!isWithinMatrix(x, y)) {
                continue;
            }
            neighbors.insert({x, y});
        }
    }

    return neighbors;
}

bool PixelDetectorModel::areNeighbors(const Pixel::Index& seed, const Pixel::Index& entrant, const size_t distance) const {
    auto pixel_distance = [](unsigned int lhs, unsigned int rhs) { return (lhs > rhs ? lhs - rhs : rhs - lhs); };
    return (pixel_distance(seed.x(), entrant.x()) <= distance && pixel_distance(seed.y(), entrant.y()) <= distance);
}
