/**
 * @file
 * @brief Implementation of radial strip detector model
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "RadialStripDetectorModel.hpp"

#include <Math/RotationZ.h>
#include <Math/Transform3D.h>

using namespace allpix;

RadialStripDetectorModel::RadialStripDetectorModel(std::string type, const ConfigReader& reader)
    : DetectorModel(std::move(type), reader) {
    auto config = reader.getHeaderConfiguration();

    // Set geometry parameters from config file
    setNumberOfStrips(config.getArray<unsigned int>("number_of_strips"));
    setStripLength(config.getArray<double>("strip_length"));
    setAngularPitch(config.getArray<double>("angular_pitch"));
    setInnerPitch(config.getArray<double>("inner_pitch"));

    // Get the number of strip rows
    auto strip_rows = static_cast<unsigned int>(number_of_strips_.size());

    // Check if parameters are defined for each strip row
    if(strip_length_.size() != strip_rows || angular_pitch_.size() != strip_rows || inner_pitch_.size() != strip_rows) {
        throw InvalidCombinationError(config,
                                      {"number_of_strips", "strip_length", "angular_pitch", "inner_pitch"},
                                      "The number of parameter values does not match the number of strip rows.");
    }

    // Dimension checks
    for(unsigned int row = 0; row < strip_rows; row++) {
        // Check if strip pitch is smaller than the length
        if(inner_pitch_.at(row) > strip_length_.at(row)) {
            throw InvalidValueError(
                config, "inner_pitch", "Inner pitch in row " + std::to_string(row) + " is larger than strip length.");
        }
        // Check that sensor segment doesn't subtend too large an angle,
        auto angle = angular_pitch_.at(row) * number_of_strips_.at(row);
        if(angle > TMath::Pi() / 2) {
            throw InvalidValueError(config, "angular_pitch", "Wafer cannot subtend a larger angle than pi/2.");
        }
        row_angle_.push_back(angle);
    }

    // Calculations of trapezoidal wrapper dimensions
    // Total strip length
    auto total_strip_length = std::accumulate(strip_length_.begin(), strip_length_.end(), 0);
    // Distance from wrapper inner edge to its focal point
    auto radius_extension = inner_pitch_.at(0) / (2 * tan(0.5 * angular_pitch_.at(0)));
    // Maximum angle subtended by the widest strip row
    auto max_angle = getRowAngleMax();
    // Distance from inner radius to wrapper inner edge
    auto strip_extension = radius_extension * (1 - cos(max_angle / 2));

    // Set the smaller base of the trapezoidal wrapper
    setSensorBaseInner(2 * radius_extension * sin(max_angle / 2));
    // Set the larger base of the trapezoidal wrapper
    setSensorBaseOuter(2 * (radius_extension + total_strip_length) * sin(max_angle / 2));
    // Set the total length of the trapezoidal wrapper
    setSensorLength(total_strip_length + strip_extension);

    // Set segment radii
    // First segment radius is the strip extension and radius extension combined
    row_radius_.push_back(strip_extension + radius_extension);
    for(unsigned int row = 0; row < strip_rows; row++) {
        // Subsequent segment radii calculated as previous segment radius plus strip length
        row_radius_.push_back(row_radius_.at(row) + strip_length_.at(row));
    }

    // Set number of strips; x-value is the maximum number of strips
    // in all rows, y-value is the number of rows
    setNPixels({*std::max_element(number_of_strips_.begin(), number_of_strips_.end()), strip_rows});
    // Pixel size is defined as the rectangular wrapper size divided by the maximum
    // number of strips (x-value) or strip rows (y-value)
    setPixelSize({getSize().x() / number_of_pixels_.x(), getSize().y() / number_of_pixels_.y()});
    // Set implant size; for now disallow setting this parameter
    auto implant_size = config.get<ROOT::Math::XYVector>("implant_size", pixel_size_);
    if(implant_size != pixel_size_) {
        throw InvalidCombinationError(
            config, {"type", "implant_size"}, "Implant size parameter is not supported for radial_strip models.");
    }
    setImplantSize(pixel_size_);

    // Set the geometrical focus point of the sensor (local coordinate center)
    setStripFocus({0, getSize().y() / 2 + radius_extension, 0});
}

bool RadialStripDetectorModel::isWithinSensor(const ROOT::Math::XYZPoint& local_pos) const {
    // Convert local position to polar coordinates
    auto polar_pos = getPositionPolar(local_pos);
    // Check if radial coordinate is outside the sensor
    if(2 * std::fabs(local_pos.z() - getSensorCenter().z()) > getSensorSize().z() ||
       (polar_pos.r() > row_radius_.back() || polar_pos.r() < row_radius_.front())) {
        return false;
    }
    // Find which strip row the position belongs to
    for(unsigned int row = 0; row < getNPixels().y(); row++) {
        if(polar_pos.r() > row_radius_.at(row) && polar_pos.r() <= row_radius_.at(row + 1)) {
            // Check if the angular coordinate is within that strip row
            return (std::fabs(polar_pos.phi()) <= angular_pitch_.at(row) * number_of_strips_.at(row) / 2);
        }
    }
    return false;
}

bool RadialStripDetectorModel::isWithinImplant(const ROOT::Math::XYZPoint& local_pos) const {
    // Convert local position to polar coordinates
    auto polar_pos = getPositionPolar(local_pos);

    // Get polar coordinates of the corresponding strip center
    auto [xstrip, ystrip] = getPixelIndex(local_pos);
    auto strip_center = getPixelCenter(static_cast<unsigned int>(xstrip), static_cast<unsigned int>(ystrip));
    auto strip_center_polar = getPositionPolar(strip_center);

    return (polar_pos.phi() * sin(std::fabs(strip_center_polar.phi() - polar_pos.phi())) < implant_size_.x() / 2);
}

bool RadialStripDetectorModel::isWithinMatrix(const Pixel::Index& strip_index) const {
    return !(strip_index.y() >= getNPixels().y() || strip_index.x() >= number_of_strips_.at(strip_index.y()));
}

ROOT::Math::XYZPoint RadialStripDetectorModel::getPixelCenter(unsigned int x, unsigned int y) const {
    // Calculate the radial coordinate of the strip center
    auto local_r = (row_radius_.at(y) + row_radius_.at(y + 1)) / 2;
    // Calculate the angular coordinate of the strip center
    auto local_phi = -angular_pitch_.at(y) * number_of_strips_.at(y) / 2 + (x + 0.5) * angular_pitch_.at(y);

    // Convert strip center position to cartesian coordinates
    auto center = getPositionCartesian(ROOT::Math::Polar2DPoint(local_r, local_phi));
    auto local_x = center.x();
    auto local_y = center.y();
    auto local_z = getSensorCenter().z() - getSensorSize().z() / 2.0;

    return {local_x, local_y, local_z};
}

std::pair<int, int> RadialStripDetectorModel::getPixelIndex(const ROOT::Math::XYZPoint& position) const {
    // Convert local position to polar coordinates
    auto polar_pos = getPositionPolar(position);

    // Get row index
    unsigned int strip_y{};
    for(unsigned int row = 0; row < getNPixels().y(); row++) {
        // Find the correct strip row by comparing to inner and outer row radii
        if(polar_pos.r() > row_radius_.at(row) && polar_pos.r() <= row_radius_.at(row + 1)) {
            strip_y = row;
            break;
        }
    }
    // Get the strip pitch in the correct strip row
    auto pitch = angular_pitch_.at(strip_y);
    // Calculate the strip x-index
    auto strip_x = static_cast<int>(std::floor((polar_pos.phi() + pitch * number_of_strips_.at(strip_y) / 2) / pitch));

    return {strip_x, static_cast<int>(strip_y)};
}

std::set<Pixel::Index> RadialStripDetectorModel::getNeighbors(const Pixel::Index& idx, const size_t distance) const {
    // Vector to hold the neighbor indices
    std::vector<Pixel::Index> neighbors;

    // Position of the global seed in polar coordinates
    auto seed_pol = getPositionPolar(getPixelCenter(idx.x(), idx.y()));
    auto idx_y = static_cast<int>(idx.y());

    // Iterate over eligible strip rows
    for(int y = static_cast<int>(-distance); y <= static_cast<int>(distance); y++) {
        // Skip row if outside of pixel matrix
        if(!isWithinMatrix(0, idx_y + y)) {
            continue;
        }

        // Set starting position of a row seed to the global seed position
        auto row_seed_r = seed_pol.r();

        // Move row seed position to the center of a requested row
        for(unsigned int shift_y = 1; shift_y <= std::labs(y); shift_y++) {
            // Add or subtract position based on whether given row is below or above global seed
            row_seed_r += (y < 0) ? -strip_length_.at(idx.y() - shift_y + 1) / 2 - strip_length_.at(idx.y() - shift_y) / 2
                                  : strip_length_.at(idx.y() + shift_y - 1) / 2 + strip_length_.at(idx.y() + shift_y) / 2;
        }

        // Get cartesian position and pixel indices of the row seed
        auto row_seed = getPositionCartesian({row_seed_r, seed_pol.phi()});
        auto [row_seed_x, row_seed_y] = getPixelIndex({row_seed.x(), row_seed.y(), 0});

        // Iterate over potential neighbors of the row seed
        for(int j = static_cast<int>(-distance); j <= static_cast<int>(distance); j++) {
            // Add to final neighbors if strip is within the pixel matrix
            if(isWithinMatrix(row_seed_x + j, row_seed_y)) {
                neighbors.push_back({static_cast<unsigned int>(row_seed_x + j), static_cast<unsigned int>(row_seed_y)});
            }
        }
    }

    return {neighbors.begin(), neighbors.end()};
}

bool RadialStripDetectorModel::areNeighbors(const Pixel::Index& seed,
                                            const Pixel::Index& entrant,
                                            const size_t distance) const {
    // y-index distance between the seed and the entrant
    auto dist_y = static_cast<int>(entrant.y()) - static_cast<int>(seed.y());

    // Seed and entrant in the same strip row
    if(dist_y == 0) {
        // Compare x-index distance to the requested distance
        return (static_cast<unsigned int>(std::labs(static_cast<int>(seed.x()) - static_cast<int>(entrant.x()))) <=
                distance);
    }

    // Position of the global seed in polar coordinates
    auto seed_pol = getPositionPolar(getPixelCenter(seed.x(), seed.y()));

    // Set starting position of a row seed to the global seed position
    auto row_seed_r = seed_pol.r();

    // Move row seed position to the center of the requested row
    for(unsigned int shift_y = 0; shift_y < std::labs(dist_y); shift_y++) {
        row_seed_r += (dist_y < 0) ? -strip_length_.at(seed.y() - shift_y) / 2 - strip_length_.at(seed.y() - shift_y - 1) / 2
                                   : strip_length_.at(seed.y() + shift_y) / 2 + strip_length_.at(seed.y() + shift_y + 1) / 2;
    }

    // Get cartesian position and pixel indices of the row seed
    auto row_seed = getPositionCartesian({row_seed_r, seed_pol.phi()});
    auto [row_seed_x, row_seed_y] = getPixelIndex({row_seed.x(), row_seed.y(), 0});

    // Compare row seed and entrant positions
    return (static_cast<unsigned int>(std::labs(row_seed_x - static_cast<int>(entrant.x()))) <= distance) &&
           (static_cast<unsigned int>(std::labs(dist_y)) <= distance);
}
