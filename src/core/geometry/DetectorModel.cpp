/**
 * @file
 * @brief Implementation of detector model
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "DetectorModel.hpp"
#include "core/module/exceptions.h"

#include "core/geometry/HexagonalPixelDetectorModel.hpp"
#include "core/geometry/PixelDetectorModel.hpp"
#include "core/geometry/RadialStripDetectorModel.hpp"

#include <Math/Translation3D.h>

using namespace allpix;

std::shared_ptr<DetectorModel> DetectorModel::factory(const std::string& name, const ConfigReader& reader) {
    Configuration config = reader.getHeaderConfiguration();

    // Sensor geometry
    // FIXME we might want to deprecate this default at some point?
    if(!config.has("geometry")) {
        LOG(WARNING) << "Model file " << config.getFilePath() << " does not provide a geometry parameter, using default";
    }
    auto geometry = config.get<std::string>("geometry", "pixel");

    // Assembly type
    if(!config.has("type")) {
        LOG(FATAL) << "Model file " << config.getFilePath() << " does not provide a type parameter";
    }
    auto type = config.get<std::string>("type");

    std::shared_ptr<DetectorAssembly> assembly;
    if(type == "hybrid") {
        assembly = std::make_shared<HybridAssembly>(reader);
    } else if(type == "monolithic") {
        assembly = std::make_shared<MonolithicAssembly>(reader);
    } else {
        LOG(FATAL) << "Model file " << config.getFilePath() << " type parameter is not valid";
        throw InvalidValueError(config, "type", "model type is not supported");
    }

    // Instantiate the correct detector model
    if(geometry == "pixel") {
        return std::make_shared<PixelDetectorModel>(name, assembly, reader);
    } else if(geometry == "radial_strip") {
        return std::make_shared<RadialStripDetectorModel>(name, assembly, reader);
    } else if(geometry == "hexagonal") {
        return std::make_shared<HexagonalPixelDetectorModel>(name, assembly, reader);
    }

    LOG(FATAL) << "Model file " << config.getFilePath() << " geometry parameter is not valid";
    // FIXME: The model can probably be silently ignored if we have more model readers later
    throw InvalidValueError(config, "geometry", "model geometry is not supported");
}

DetectorModel::DetectorModel(std::string type, std::shared_ptr<DetectorAssembly> assembly, ConfigReader reader)
    : type_(std::move(type)), assembly_(std::move(assembly)), reader_(std::move(reader)) {
    using namespace ROOT::Math;
    auto config = reader_.getHeaderConfiguration();

    // Sensor thickness
    setSensorThickness(config.get<double>("sensor_thickness"));

    // Excess around the sensor from the pixel grid
    auto default_sensor_excess = config.get<double>("sensor_excess", 0);
    setSensorExcessTop(config.get<double>("sensor_excess_top", default_sensor_excess));
    setSensorExcessBottom(config.get<double>("sensor_excess_bottom", default_sensor_excess));
    setSensorExcessLeft(config.get<double>("sensor_excess_left", default_sensor_excess));
    setSensorExcessRight(config.get<double>("sensor_excess_right", default_sensor_excess));

    // Sensor material:
    sensor_material_ = config.get<SensorMaterial>("sensor_material", SensorMaterial::SILICON);

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

ROOT::Math::XYZPoint DetectorModel::getModelCenter() const {

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
    return ROOT::Math::XYZPoint(getMatrixCenter().x(), getMatrixCenter().y(), center);
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
    size.SetX(2 * std::max(max.x() - getMatrixCenter().x(), getMatrixCenter().x() - min.x()));
    size.SetY(2 * std::max(max.y() - getMatrixCenter().y(), getMatrixCenter().y() - min.y()));
    size.SetZ((max.z() - getMatrixCenter().z()) +
              (getMatrixCenter().z() - min.z())); // max.z() is positive (chip side) and min.z() is negative (sensor side)

    // FIXME need a better solution than this!
    auto assembly = std::dynamic_pointer_cast<HybridAssembly>(getAssembly());
    if(assembly != nullptr) {
        auto bump_grid = getSensorSize() + 2 * ROOT::Math::XYZVector(std::fabs(assembly->getBumpsOffset().x()),
                                                                     std::fabs(assembly->getBumpsOffset().y()),
                                                                     0);

        // Extend size unless it's already large enough to cover shifted bump bond grid:
        return ROOT::Math::XYZVector(
            std::max(size.x(), bump_grid.x()), std::max(size.y(), bump_grid.y()), std::max(size.z(), bump_grid.z()));
    }
    return size;
}

std::vector<SupportLayer> DetectorModel::getSupportLayers() const {
    auto ret_layers = support_layers_;

    auto sensor_offset = -getSensorSize().z() / 2.0;
    auto chip_offset = getSensorSize().z() / 2.0 + getChipSize().z() + assembly_->getChipOffset().z();
    for(auto& layer : ret_layers) {
        ROOT::Math::XYZVector offset = layer.offset_;
        if(layer.location_ == "sensor") {
            offset.SetZ(sensor_offset - layer.size_.z() / 2.0);
            sensor_offset -= layer.size_.z();
        } else if(layer.location_ == "chip") {
            offset.SetZ(chip_offset + layer.size_.z() / 2.0);
            chip_offset += layer.size_.z();
        }

        layer.center_ = getMatrixCenter() + offset;
    }

    return ret_layers;
}

ROOT::Math::XYZPoint DetectorModel::getSensorIntercept(const ROOT::Math::XYZPoint& inside,
                                                       const ROOT::Math::XYZPoint& outside) const {
    // Get direction vector of motion
    auto direction = (outside - inside).Unit();
    // We have to be centered around the sensor box. This means we need to shift by the matrix center
    auto translation_local = ROOT::Math::Translation3D(static_cast<ROOT::Math::XYZVector>(getMatrixCenter()));
    auto pos_in = translation_local.Inverse()(inside);

    return translation_local(liang_barsky_clipping(direction, pos_in, getSensorSize()));
}

ROOT::Math::XYZPoint DetectorModel::liang_barsky_clipping(const ROOT::Math::XYZVector& direction,
                                                          const ROOT::Math::XYZPoint& position,
                                                          const ROOT::Math::XYZVector& box) {

    auto clip = [](double denominator, double numerator, double& t0, double& t1) {
        if(denominator > 0) {
            if(numerator > denominator * t1) {
                return false;
            }
            if(numerator > denominator * t0) {
                t0 = numerator / denominator;
            }
            return true;
        } else if(denominator < 0) {
            if(numerator > denominator * t0) {
                return false;
            }
            if(numerator > denominator * t1) {
                t1 = numerator / denominator;
            }
            return true;
        } else {
            return numerator <= 0;
        }
    };

    // Clip the particle track against the six possible box faces
    double t0 = std::numeric_limits<double>::lowest(), t1 = std::numeric_limits<double>::max();
    bool intersect = clip(direction.X(), -position.X() - box.X() / 2, t0, t1) &&
                     clip(-direction.X(), position.X() - box.X() / 2, t0, t1) &&
                     clip(direction.Y(), -position.Y() - box.Y() / 2, t0, t1) &&
                     clip(-direction.Y(), position.Y() - box.Y() / 2, t0, t1) &&
                     clip(direction.Z(), -position.Z() - box.Z() / 2, t0, t1) &&
                     clip(-direction.Z(), position.Z() - box.Z() / 2, t0, t1);

    // The intersection is a point P + t * D. Return closest impact point if positive (i.e. in direction of the motion)
    if(intersect) {
        if(t0 > 0 && t1 > 0) {
            return (position + std::min(t0, t1) * direction);
        } else if(t0 > 0) {
            return (position + t0 * direction);
        } else if(t1 > 0) {
            return (position + t1 * direction);
        }
    }

    // Otherwise: The line does not intersect the box.
    throw std::invalid_argument("one point needs to be outside and one inside the sensor volume");
}

ROOT::Math::XYZPoint DetectorModel::getImplantIntercept(const ROOT::Math::XYZPoint& outside,
                                                        const ROOT::Math::XYZPoint& inside) const {
    // Get direction vector of motion
    auto direction = (inside - outside).Unit();
    // Get positions relative to pixel center:
    auto [xpixel_out, ypixel_out] = getPixelIndex(outside);
    // We have to be centered around the implant box, not the pixel. This means we need to shift with the implant offset
    auto translation_px = ROOT::Math::Translation3D(
        static_cast<ROOT::Math::XYZVector>(getPixelCenter(xpixel_out, ypixel_out) + getImplantOffset()));
    auto pos_out = translation_px.Inverse()(outside);

    return translation_px(liang_barsky_clipping(direction, pos_out, getImplantSize()));
}
