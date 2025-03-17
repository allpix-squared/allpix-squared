/**
 * @file
 * @brief Implementation of a pixel detector model
 *
 * @copyright Copyright (c) 2021-2025 CERN and the Allpix Squared authors.
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
                                       const ConfigReader& reader,
                                       const Configuration& config)
    : DetectorModel(std::move(type), assembly, reader, config) {
    using namespace ROOT::Math;

    // Number of pixels
    setNPixels(config.get<DisplacementVector2D<Cartesian2D<unsigned int>>>("number_of_pixels"));
    // Size of the pixels
    auto pixel_size = config.get<XYVector>("pixel_size");
    setPixelSize(std::move(pixel_size));
}

void PixelDetectorModel::validate() {

    // Validate implants:
    for(const auto& implant : this->getImplants()) {
        if(implant.getSize().x() > pixel_size_.x() || implant.getSize().y() > pixel_size_.y()) {
            throw InvalidValueError(implant.getConfiguration(), "size", "implant size cannot be larger than pixel pitch");
        }
        if(implant.getSize().z() > getSensorSize().z()) {
            throw InvalidValueError(
                implant.getConfiguration(), "size", "implant depth cannot be larger than sensor thickness");
        }

        if(implant.getType() == Implant::Type::BACKSIDE) {
            // For backside implants, only check that the center of the implant lies within the pixel cell:
            if(std::fabs(implant.getOffset().x()) > pixel_size_.x() / 2 ||
               std::fabs(implant.getOffset().y()) > pixel_size_.y() / 2) {
                throw InvalidValueError(
                    implant.getConfiguration(), "offset", "implant offset outside cell. Reduce implant offset");
            }
        } else {
            // For frontside implants, check that the implant lies within the pixel cell with its entire size
            if(std::fabs(implant.getOffset().x()) + implant.getSize().x() / 2 > pixel_size_.x() / 2 ||
               std::fabs(implant.getOffset().y()) + implant.getSize().y() / 2 > pixel_size_.y() / 2) {
                throw InvalidValueError(
                    implant.getConfiguration(), "offset", "implant exceeds pixel cell. Reduce implant size or offset");
            }
        }
    }
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
 * The definition of the sensor boundary is determined by the detector model
 */
bool PixelDetectorModel::isOnSensorBoundary(const ROOT::Math::XYZPoint& local_pos) const {
    auto sensor_size = getSensorSize();
    return (2 * std::fabs(local_pos.z()) == sensor_size.z()) || (2 * std::fabs(local_pos.y()) == sensor_size.y()) ||
           (2 * std::fabs(local_pos.x()) == sensor_size.x());
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

/**
 * Faster implementation of matrix lookup for local coordinate positions than going through the pixel index
 * This is quite easy for rectangular pixels and matrices.
 */
bool PixelDetectorModel::isWithinMatrix(const ROOT::Math::XYZPoint& position) const {
    if(position.x() < -0.5 * pixel_size_.x() || position.x() > (number_of_pixels_.x() - 0.5) * pixel_size_.x()) {
        return false;
    }
    if(position.y() < -0.5 * pixel_size_.y() || position.y() > (number_of_pixels_.y() - 0.5) * pixel_size_.y()) {
        return false;
    }
    return true;
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
