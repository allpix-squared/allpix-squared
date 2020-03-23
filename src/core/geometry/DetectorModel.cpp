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
        auto hole_size = support_config.get<XYVector>("hole_size", {0, 0});
        auto hole_offset = support_config.get<XYVector>("hole_offset", {0, 0});
        addSupportLayer(size, thickness, offset, material, location, hole_size, hole_offset);
    }
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
