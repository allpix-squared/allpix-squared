/**
 * @file
 * @brief Implementation of detector model
 *
 * @copyright Copyright (c) 2017-2023 CERN and the Allpix Squared authors.
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
#include "tools/liang_barsky.h"

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
        assembly = std::make_shared<HybridAssembly>(config);
    } else if(type == "monolithic") {
        assembly = std::make_shared<MonolithicAssembly>(config);
    } else {
        LOG(FATAL) << "Model file " << config.getFilePath() << " type parameter is not valid";
        throw InvalidValueError(config, "type", "model type is not supported");
    }

    // Instantiate the correct detector model
    std::shared_ptr<DetectorModel> model;
    if(geometry == "pixel") {
        model = std::make_shared<PixelDetectorModel>(name, assembly, reader, config);
    } else if(geometry == "radial_strip") {
        model = std::make_shared<RadialStripDetectorModel>(name, assembly, reader, config);
    } else if(geometry == "hexagonal") {
        model = std::make_shared<HexagonalPixelDetectorModel>(name, assembly, reader, config);
    } else {
        LOG(FATAL) << "Model file " << config.getFilePath() << " geometry parameter is not valid";
        // FIXME: The model can probably be silently ignored if we have more model readers later
        throw InvalidValueError(config, "geometry", "model geometry is not supported");
    }

    // Validate the detector model - we call this here because validation might depend on derived class properties:
    model->validate();

    auto unused_keys = config.getUnusedKeys();
    if(!unused_keys.empty()) {
        std::stringstream st;
        st << "Unused configuration keys in global section of sensor geometry definition:";
        for(auto& key : unused_keys) {
            st << std::endl << key;
        }
        LOG(WARNING) << st.str();
    }

    return model;
}

DetectorModel::DetectorModel(std::string type,
                             std::shared_ptr<DetectorAssembly> assembly,
                             const ConfigReader& reader,
                             const Configuration& config)
    : type_(std::move(type)), assembly_(std::move(assembly)), reader_(reader) {
    using namespace ROOT::Math;

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

    // Issue a warning for pre-3.0 implant definitions:
    if(config.has("implant_size")) {
        LOG(WARNING) << "Parameter \"implant_size\" of model " << config.getFilePath() << " not supported," << std::endl
                     << "Individual [implant] sections must be used for implant definitions";
    }

    // Read implants
    for(auto& implant_config : reader_.getConfigurations("implant")) {
        auto imtype = implant_config.get<Implant::Type>("type");
        auto shape = implant_config.get<Implant::Shape>("shape", Implant::Shape::RECTANGLE);
        auto size = implant_config.get<XYZVector>("size");
        auto offset = implant_config.get<XYVector>("offset", {0, 0});
        auto orientation = implant_config.get<double>("orientation", 0.);

        addImplant(imtype, shape, std::move(size), offset, orientation, implant_config);

        auto unused_keys = implant_config.getUnusedKeys();
        if(!unused_keys.empty()) {
            std::stringstream st;
            st << "Unused configuration keys in [implant] section of sensor geometry definition:";
            for(auto& key : unused_keys) {
                st << std::endl << key;
            }
            LOG(WARNING) << st.str();
        }
    }

    // Read support layers
    LOG(DEBUG) << "Number of [support] sections: " << reader_.getConfigurations("support").size();
    for(auto& support_config : reader_.getConfigurations("support")) {
        auto thickness = support_config.get<double>("thickness");
        auto size = support_config.get<XYVector>("size");
        auto location = support_config.get<SupportLayer::Location>("location", SupportLayer::Location::CHIP);

        XYZVector offset;
        if(location == SupportLayer::Location::ABSOLUTE) {
            offset = support_config.get<XYZVector>("offset");
        } else {
            auto xy_offset = support_config.get<XYVector>("offset", {0, 0});
            offset = XYZVector(xy_offset.x(), xy_offset.y(), 0);
        }

        auto material = support_config.get<std::string>("material", "g10");
        auto hole_type = support_config.get<std::string>("hole_type", "rectangular");
        std::transform(hole_type.begin(), hole_type.end(), hole_type.begin(), ::tolower);
        auto hole_size = support_config.get<XYVector>("hole_size", {0, 0});
        auto hole_offset = support_config.get<XYVector>("hole_offset", {0, 0});
        addSupportLayer(size,
                        thickness,
                        std::move(offset),
                        std::move(material),
                        std::move(hole_type),
                        location,
                        hole_size,
                        std::move(hole_offset));

        auto unused_keys = support_config.getUnusedKeys();
        if(!unused_keys.empty()) {
            std::stringstream st;
            st << "Unused configuration keys in [support] section of sensor geometry definition:";
            for(auto& key : unused_keys) {
                st << std::endl << key;
            }
            LOG(WARNING) << st.str();
        }
    }
}

void DetectorModel::addImplant(const Implant::Type& type,
                               const Implant::Shape& shape,
                               ROOT::Math::XYZVector size,
                               const ROOT::Math::XYVector& offset,
                               double orientation,
                               const Configuration& config) {
    // Calculate offset from sensor center - sign of the shift depends on whether it's on front- or backside:
    auto offset_z = (getSensorSize().z() - size.z()) / 2. * (type == Implant::Type::FRONTSIDE ? 1 : -1);
    ROOT::Math::XYZVector full_offset(offset.x(), offset.y(), offset_z);
    implants_.push_back(
        Implant(type, shape, std::move(size), std::move(full_offset), ROOT::Math::RotationZ(orientation), config));
}

void DetectorModel::validate() {
    // FIXME at some point we might make this a requirement and throw an exception instead?
    LOG(WARNING) << "No validation implemented for this detector geometry";
}

bool DetectorModel::Implant::contains(const ROOT::Math::XYZVector& position) const {
    // Shift position to implant coordinate system and apply rotation around z axis:
    auto pos = orientation_(position - offset_);

    // Check z-position to be within implant
    if(std::fabs(pos.z()) >= size_.z() / 2) {
        return false;
    }

    if(shape_ == Implant::Shape::RECTANGLE) {
        // Check if point is within rectangle with side lengths size_
        if(std::fabs(pos.x()) <= size_.x() / 2 && std::fabs(pos.y()) <= size_.y() / 2) {
            return true;
        }
    } else if(shape_ == Implant::Shape::ELLIPSE) {
        // Check if point is within ellipsis with major axis size_
        if(pos.x() * pos.x() / (size_.x() * size_.x() / 4) + pos.y() * pos.y() / (size_.y() * size_.y() / 4) <= 1) {
            return true;
        }
    }
    return false;
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
        const auto& size = support_layer.getSize();
        const auto& center = support_layer.getCenter();
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
        if(layer.location_ == SupportLayer::Location::SENSOR) {
            offset.SetZ(sensor_offset - layer.size_.z() / 2.0);
            sensor_offset -= layer.size_.z();
        } else if(layer.location_ == SupportLayer::Location::CHIP) {
            offset.SetZ(chip_offset + layer.size_.z() / 2.0);
            chip_offset += layer.size_.z();
        }

        layer.center_ = getMatrixCenter() + offset;
    }

    return ret_layers;
}

std::optional<DetectorModel::Implant> DetectorModel::isWithinImplant(const ROOT::Math::XYZPoint& local_pos) const {

    // Bail out if we have no implants - no need to transform coordinates:
    if(implants_.empty()) {
        return std::nullopt;
    }

    auto [xpixel, ypixel] = getPixelIndex(local_pos);
    auto inPixelPos = local_pos - getPixelCenter(xpixel, ypixel);

    for(const auto& implant : implants_) {
        if(implant.contains(inPixelPos)) {
            return implant;
        }
    }
    return std::nullopt;
}

ROOT::Math::XYZPoint DetectorModel::getImplantIntercept(const Implant& implant,
                                                        const ROOT::Math::XYZPoint& outside,
                                                        const ROOT::Math::XYZPoint& inside) const {
    // Get direction vector of motion *into* implant
    auto direction = (inside - outside).Unit();
    // Get positions relative to pixel center:
    auto [xpixel_in, ypixel_in] = getPixelIndex(inside);
    auto transl = ROOT::Math::Translation3D(static_cast<ROOT::Math::XYZVector>(getPixelCenter(xpixel_in, ypixel_in)));

    // Call implant intersection and re-transform back to local coordinates:
    auto intercept = implant.intersect(direction, transl.Inverse()(outside));
    if(!intercept.has_value()) {
        return inside;
    }
    return transl(intercept.value());
}

std::optional<ROOT::Math::XYZPoint> DetectorModel::Implant::intersect(const ROOT::Math::XYZVector& direction,
                                                                      const ROOT::Math::XYZPoint& position) const {
    // Shift position to implant coordinate system and apply rotation around z axis!
    if(shape_ == Implant::Shape::RECTANGLE) {
        // Use Liang-Barsky line clipping method:
        auto intercept = LiangBarsky::closestIntersection(orientation_(direction), orientation_(position - offset_), size_);
        if(intercept.has_value()) {
            // Translate back into local coordinates of the sensor:
            intercept = orientation_.Inverse()(intercept.value()) + offset_;
        }
        return intercept;
    } else if(shape_ == Implant::Shape::ELLIPSE) {
        // Translate so the ellipse is centered at the origin.
        auto pos = orientation_(position - offset_);
        auto dir = orientation_(direction);

        // Normal vector for top and bottom faces of cylinder
        const auto norm = ROOT::Math::XYZVector(0, 0, 1);

        auto intersection_caps = [&](const ROOT::Math::XYZVector& p, const ROOT::Math::XYZVector& d) {
            auto d1 =
                (ROOT::Math::XYZVector(0, 0, size_.z() / 2) - static_cast<ROOT::Math::XYZVector>(p)).Dot(norm) / d.Dot(norm);
            auto d2 = (ROOT::Math::XYZVector(0, 0, -size_.z() / 2) - static_cast<ROOT::Math::XYZVector>(p)).Dot(norm) /
                      d.Dot(norm);
            // Return the smaller d value, closer to the reference point
            return std::min(d1, d2);
        };

        // Calculate quadratic parameters with semimajor/minor axes (half-diameter) and discriminant.
        double A = 4 * dir.x() * dir.x() / size_.x() / size_.x() + 4 * dir.y() * dir.y() / size_.y() / size_.y();
        double B = 8 * pos.x() * dir.x() / size_.x() / size_.x() + 8 * pos.y() * dir.y() / size_.y() / size_.y();
        double C = 4 * pos.x() * pos.x() / size_.x() / size_.x() + 4 * pos.y() * pos.y() / size_.y() / size_.y() - 1;
        auto discriminant = B * B - 4 * A * C;

        // No intersection with negative discriminant:
        if(discriminant < 0) {
            return std::nullopt;
        }

        auto t1 = (-B - std::sqrt(discriminant));
        auto t2 = (-B + std::sqrt(discriminant));

        // Two intersections and both are in direction of motion
        if(discriminant > 0 && t1 > 0 && t2 > 0) {
            auto intersect = pos + dir * std::min(t1, t2) / 2 / A;
            // Closer solution hits cylinder wall, return as intersection
            if(std::fabs(intersect.z()) < size_.z() / 2) {
                return orientation_.Inverse()(intersect) + offset_;
            }
        }

        // Only one solution - either discriminant is 0 or one solution is in negative direction of motion.
        // We only check for cylinder cap intersections here, pure contact solutions (discr = 0) are ignored
        auto t = intersection_caps(static_cast<ROOT::Math::XYZVector>(pos), dir);
        auto intersection = pos + dir * t;

        // Check if solution found is within cylinder area, i.e. end cap:
        if(intersection.x() * intersection.x() / (size_.x() * size_.x() / 4) +
               intersection.y() * intersection.y() / (size_.y() * size_.y() / 4) <=
           1) {
            return orientation_.Inverse()(intersection) + offset_;
        }

        // No intersection or only contact point found.
        return std::nullopt;
    }
    throw std::invalid_argument("Intersection not implemented for this implant type");
}
