/**
 * @file
 * @brief Implementation of detector model
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "DetectorModel.hpp"
#include "core/module/exceptions.h"

using namespace allpix;

DetectorModel::DetectorModel(std::string type, ConfigReader reader) : type_(std::move(type)), reader_(std::move(reader)) {
    using namespace ROOT::Math;
    auto config = reader_.getHeaderConfiguration();

    // Number of pixels
    setNPixels(config.get<DisplacementVector2D<Cartesian2D<unsigned int>>>("number_of_pixels"));
    // Size of the pixels
    auto pixel_size = config.get<XYVector>("pixel_size");
    setPixelSize(pixel_size);
    // Size of the collection diode implant on each pixels, defaults to the full pixel size when not specified
    auto implant_size = config.get<XYVector>("implant_size", pixel_size);
    if(implant_size.x() > pixel_size.x() || implant_size.y() > pixel_size.y()) {
        throw InvalidValueError(config, "implant_size", "implant size cannot be larger than pixel pitch");
    }
    setImplantSize(implant_size);

    // Sensor thickness
    setSensorThickness(config.get<double>("sensor_thickness"));
    // Excess around the sensor from the pixel grid
    auto default_sensor_excess = config.get<double>("sensor_excess", 0);
    setSensorExcessTop(config.get<double>("sensor_excess_top", default_sensor_excess));
    setSensorExcessBottom(config.get<double>("sensor_excess_bottom", default_sensor_excess));
    setSensorExcessLeft(config.get<double>("sensor_excess_left", default_sensor_excess));
    setSensorExcessRight(config.get<double>("sensor_excess_right", default_sensor_excess));

    // Chip thickness
    setChipThickness(config.get<double>("chip_thickness", 0));

    // Read support layers
    for(auto& support_config : reader_.getConfigurations("support")) {
        auto thickness = support_config.get<double>("thickness");
        auto size = support_config.get<XYVector>("size");
        auto location = support_config.get<std::string>("location", "chip");
        std::transform(location.begin(), location.end(), location.begin(), ::tolower);
        if(location != "sensor" && location != "chip" && location != "absolute") {
            throw InvalidValueError(
                support_config, "location", "location of the support should be 'chip', 'sensor' or 'absolute'");
        }
        XYZVector offset;
        if(location == "absolute") {
            offset = support_config.get<XYZVector>("offset");
        } else {
            auto xy_offset = support_config.get<XYVector>("offset", {0, 0});
            offset = XYZVector(xy_offset.x(), xy_offset.y(), 0);
        }

        auto material = support_config.get<std::string>("material", "g10");
        std::transform(material.begin(), material.end(), material.begin(), ::tolower);
        auto hole_type = support_config.get<std::string>("hole_type", "rectangular");
        std::transform(hole_type.begin(), hole_type.end(), hole_type.begin(), ::tolower);
        auto hole_size = support_config.get<XYVector>("hole_size", {0, 0});
        auto hole_offset = support_config.get<XYVector>("hole_offset", {0, 0});
        addSupportLayer(size, thickness, offset, material, hole_type, location, hole_size, hole_offset);
    }
}

ROOT::Math::XYZPoint DetectorModel::getGeometricalCenter() const {

    // Prepare detector assembly stack (sensor, chip, supports) with z-positions and thicknesses:
    std::vector<std::pair<double, double>> stack = {{getSensorCenter().z(), getSensorSize().z()},
                                                    {getChipCenter().z(), getChipSize().z()}};
    for(auto& support_layer : getSupportLayers()) {
        stack.emplace_back(support_layer.getCenter().z(), support_layer.getSize().z());
    }

    // Find first and last element of detector assembly stack:
    auto boundaries = std::minmax_element(
        stack.begin(), stack.end(), [](std::pair<double, double> const& s1, std::pair<double, double> const& s2) {
            return s1.first < s2.first;
        });

    auto element_first = *boundaries.first;
    auto element_last = *boundaries.second;

    // Calculate geometrical center as mid-point between boundaries (first element minus half thickness, last element plus
    // half thickness)
    auto center =
        ((element_first.first - element_first.second / 2.0) + (element_last.first + element_last.second / 2.0)) / 2.0;
    return ROOT::Math::XYZPoint(getCenter().x(), getCenter().y(), center);
}

std::vector<Configuration> DetectorModel::getConfigurations() const {
    std::vector<Configuration> configurations;
    // Initialize global base configuration
    auto global_config_ = reader_.getHeaderConfiguration();

    for(auto& config : reader_.getConfigurations()) {
        if(config.getName().empty()) {
            // Merge all global sections with the global config
            global_config_.merge(config);
        } else {
            // Store all others
            configurations.push_back(config);
        }
    }

    // Prepend global config and return vector:
    configurations.insert(configurations.begin(), global_config_);
    return configurations;
}

ROOT::Math::XYZVector DetectorModel::getSize() const {
    ROOT::Math::XYZVector max(
        std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest());
    ROOT::Math::XYZVector min(
        std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), std::numeric_limits<double>::max());

    std::array<ROOT::Math::XYZPoint, 2> centers = {{getSensorCenter(), getChipCenter()}};
    std::array<ROOT::Math::XYZVector, 2> sizes = {{getSensorSize(), getChipSize()}};

    for(size_t i = 0; i < 2; ++i) {
        max.SetX(std::max(max.x(), (centers.at(i) + sizes.at(i) / 2.0).x()));
        max.SetY(std::max(max.y(), (centers.at(i) + sizes.at(i) / 2.0).y()));
        max.SetZ(std::max(max.z(), (centers.at(i) + sizes.at(i) / 2.0).z()));
        min.SetX(std::min(min.x(), (centers.at(i) - sizes.at(i) / 2.0).x()));
        min.SetY(std::min(min.y(), (centers.at(i) - sizes.at(i) / 2.0).y()));
        min.SetZ(std::min(min.z(), (centers.at(i) - sizes.at(i) / 2.0).z()));
    }

    for(auto& support_layer : getSupportLayers()) {
        auto size = support_layer.getSize();
        auto center = support_layer.getCenter();
        max.SetX(std::max(max.x(), (center + size / 2.0).x()));
        max.SetY(std::max(max.y(), (center + size / 2.0).y()));
        max.SetZ(std::max(max.z(), (center + size / 2.0).z()));
        min.SetX(std::min(min.x(), (center - size / 2.0).x()));
        min.SetY(std::min(min.y(), (center - size / 2.0).y()));
        min.SetZ(std::min(min.z(), (center - size / 2.0).z()));
    }

    ROOT::Math::XYZVector size;
    size.SetX(2 * std::max(max.x() - getCenter().x(), getCenter().x() - min.x()));
    size.SetY(2 * std::max(max.y() - getCenter().y(), getCenter().y() - min.y()));
    size.SetZ((max.z() - getCenter().z()) +
              (getCenter().z() - min.z())); // max.z() is positive (chip side) and min.z() is negative (sensor side)
    return size;
}

std::vector<DetectorModel::SupportLayer> DetectorModel::getSupportLayers() const {
    auto ret_layers = support_layers_;

    auto sensor_offset = -getSensorSize().z() / 2.0;
    auto chip_offset = getSensorSize().z() / 2.0 + getChipSize().z();
    for(auto& layer : ret_layers) {
        ROOT::Math::XYZVector offset = layer.offset_;
        if(layer.location_ == "sensor") {
            offset.SetZ(sensor_offset - layer.size_.z() / 2.0);
            sensor_offset -= layer.size_.z();
        } else if(layer.location_ == "chip") {
            offset.SetZ(chip_offset + layer.size_.z() / 2.0);
            chip_offset += layer.size_.z();
        }

        layer.center_ = getCenter() + offset;
    }

    return ret_layers;
}

/**
 * The definition of inside the sensor is determined by the detector model
 */
bool DetectorModel::isWithinSensor(const ROOT::Math::XYZPoint& local_pos) const {
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
bool DetectorModel::isWithinImplant(const ROOT::Math::XYZPoint& local_pos) const {

    auto [xpixel, ypixel] = getPixelIndex(local_pos);
    auto inPixelPos = local_pos - getPixelCenter(static_cast<unsigned int>(xpixel), static_cast<unsigned int>(ypixel));

    return (std::fabs(inPixelPos.x()) <= std::fabs(getImplantSize().x() / 2) &&
            std::fabs(inPixelPos.y()) <= std::fabs(getImplantSize().y() / 2));
}

/**
 * The definition of the pixel grid size is determined by the detector model
 */
bool DetectorModel::isWithinPixelGrid(const Pixel::Index& pixel_index) const {
    return !(pixel_index.x() >= number_of_pixels_.x() || pixel_index.y() >= number_of_pixels_.y());
}

/**
 * The definition of the pixel grid size is determined by the detector model
 */
bool DetectorModel::isWithinPixelGrid(const int x, const int y) const {
    return !(x < 0 || x >= static_cast<int>(number_of_pixels_.x()) || y < 0 || y >= static_cast<int>(number_of_pixels_.y()));
}

ROOT::Math::XYZPoint DetectorModel::getPixelCenter(unsigned int x, unsigned int y) const {
    auto size = getPixelSize();
    auto local_x = size.x() * x;
    auto local_y = size.y() * y;
    auto local_z = getSensorCenter().z() - getSensorSize().z() / 2.0;

    return {local_x, local_y, local_z};
}

std::pair<int, int> DetectorModel::getPixelIndex(const ROOT::Math::XYZPoint& position) const {
    auto pixel_x = static_cast<int>(std::round(position.x() / pixel_size_.x()));
    auto pixel_y = static_cast<int>(std::round(position.y() / pixel_size_.y()));
    return {pixel_x, pixel_y};
}

std::set<std::pair<int, int>> DetectorModel::getNeighborPixels(const Pixel::Index& idx,
                                                               const Pixel::Index& last_idx,
                                                               const ROOT::Math::XYVector& ind_matrix) const {
    std::set<std::pair<int, int>> neighbors;

    int x_lower = static_cast<int>(std::min(idx.x(), last_idx.x()) - std::floor(ind_matrix.x() / 2));
    int x_higher = static_cast<int>(std::max(idx.x(), last_idx.x()) + std::floor(ind_matrix.x() / 2));
    int y_lower = static_cast<int>(std::min(idx.y(), last_idx.y()) - std::floor(ind_matrix.y() / 2));
    int y_higher = static_cast<int>(std::max(idx.y(), last_idx.y()) + std::floor(ind_matrix.y() / 2));

    for(int x = x_lower; x <= x_higher; x++) {
        for(int y = y_lower; y <= y_higher; y++) {
            if(!isWithinPixelGrid(x, y)) {
                continue;
            }
            neighbors.insert({x, y});
        }
    }

    return neighbors;
}
