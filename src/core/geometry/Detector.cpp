/**
 * @file
 * @brief Implementation of detector
 *
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <Math/Rotation3D.h>
#include <Math/Translation3D.h>

#include "Detector.hpp"
#include "core/module/exceptions.h"

using namespace allpix;

/**
 * @throws InvalidModuleActionException If the detector model pointer is a null pointer
 *
 * Creates a detector without any electric field in the sensor.
 */
Detector::Detector(std::string name,
                   std::shared_ptr<DetectorModel> model,
                   ROOT::Math::XYZPoint position,
                   const ROOT::Math::Rotation3D& orientation)
    : Detector(std::move(name), std::move(position), orientation) {
    model_ = std::move(model);
    // Check if valid model is supplied
    if(model_ == nullptr) {
        throw InvalidModuleActionException("Detector model cannot be a null pointer");
    }

    magnetic_field_on_ = false;
    // Build the transformation matrix
    build_transform();
}

/**
 * This constructor can only be called directly by the \ref GeometryManager to
 * instantiate incomplete detectors where the model is added later. It is ensured
 * that these detectors can never be accessed by modules before the detector
 * model is added.
 */
Detector::Detector(std::string name, ROOT::Math::XYZPoint position, const ROOT::Math::Rotation3D& orientation)
    : name_(std::move(name)), position_(std::move(position)), orientation_(orientation) {}

void Detector::set_model(std::shared_ptr<DetectorModel> model) {
    model_ = std::move(model);
    electric_field_.set_model_parameters(model_->getSensorCenter(), model_->getSensorSize(), model_->getPixelSize());
    magnetic_field_on_ = false;
    build_transform();
}
void Detector::build_transform() {
    // Transform from locally centered to global coordinates
    ROOT::Math::Translation3D translation_center(static_cast<ROOT::Math::XYZVector>(position_));
    ROOT::Math::Rotation3D rotation_center(orientation_);
    // Transformation from locally centered into global coordinate system, consisting of
    // * The rotation into the global coordinate system
    // * The shift from the origin to the detector position
    ROOT::Math::Transform3D transform_center(rotation_center, translation_center);
    // Transform from locally centered to local coordinates
    ROOT::Math::Translation3D translation_local(static_cast<ROOT::Math::XYZVector>(model_->getCenter()));
    ROOT::Math::Transform3D transform_local(translation_local);
    // Compute total transform local to global by first transforming local to locally centered and then to global coordinates
    transform_ = transform_center * transform_local.Inverse();
}

std::string Detector::getName() const {
    return name_;
}
std::string Detector::getType() const {
    return model_->getType();
}

const std::shared_ptr<DetectorModel> Detector::getModel() const {
    return model_;
}

ROOT::Math::XYZPoint Detector::getPosition() const {
    return position_;
}
ROOT::Math::Rotation3D Detector::getOrientation() const {
    return orientation_;
}

/**
 * @warning The local coordinate position does normally not have its origin at the center of rotation
 *
 * The origin of the local frame is at the center of the first pixel in the middle of the sensor.
 */
ROOT::Math::XYZPoint Detector::getLocalPosition(const ROOT::Math::XYZPoint& global_pos) const {
    return transform_.Inverse()(global_pos);
}
ROOT::Math::XYZPoint Detector::getGlobalPosition(const ROOT::Math::XYZPoint& local_pos) const {
    return transform_(local_pos);
}

/**
 * The definition of inside the sensor is determined by the detector model
 */
bool Detector::isWithinSensor(const ROOT::Math::XYZPoint& local_pos) const {
    auto sensor_center = model_->getSensorCenter();
    auto sensor_size = model_->getSensorSize();
    return (2 * std::fabs(local_pos.z() - sensor_center.z()) <= sensor_size.z()) &&
           (2 * std::fabs(local_pos.y() - sensor_center.y()) <= sensor_size.y()) &&
           (2 * std::fabs(local_pos.x() - sensor_center.x()) <= sensor_size.x());
}

/**
 * The definition of inside the implant region is determined by the detector model
 *
 * @note The pixel implant currently is always positioned symmetrically, in the center of the pixel cell.
 */
bool Detector::isWithinImplant(const ROOT::Math::XYZPoint& local_pos) const {

    auto x_mod_pixel = std::fmod(local_pos.x() + model_->getPixelSize().x() / 2, model_->getPixelSize().x()) -
                       model_->getPixelSize().x() / 2;
    auto y_mod_pixel = std::fmod(local_pos.y() + model_->getPixelSize().y() / 2, model_->getPixelSize().y()) -
                       model_->getPixelSize().y() / 2;

    return (std::fabs(x_mod_pixel) <= std::fabs(model_->getImplantSize().x() / 2) &&
            std::fabs(y_mod_pixel) <= std::fabs(model_->getImplantSize().y() / 2));
}

/**
 * The pixel has internal information about the size and location specific for this detector
 */
Pixel Detector::getPixel(unsigned int x, unsigned int y) {
    Pixel::Index index(x, y);

    auto size = model_->getPixelSize();

    // WARNING This relies on the origin of the local coordinate system
    auto local_x = size.x() * x;
    auto local_y = size.y() * y;
    auto local_z = model_->getSensorCenter().z() - model_->getSensorSize().z() / 2.0;

    auto local_center = ROOT::Math::XYZPoint(local_x, local_y, local_z);
    auto global_center = getGlobalPosition(local_center);

    return {index, local_center, global_center, size};
}

/**
 * The electric field is replicated for all pixels and uses flipping at each boundary (side effects are not modeled in this
 * stage). Outside of the sensor the electric field is strictly zero by definition.
 */
bool Detector::hasElectricField() const {
    return electric_field_.isValid();
}

/**
 * The electric field is replicated for all pixels and uses flipping at each boundary (side effects are not modeled in this
 * stage). Outside of the sensor the electric field is strictly zero by definition.
 */
ROOT::Math::XYZVector Detector::getElectricField(const ROOT::Math::XYZPoint& pos) const {
    return electric_field_.get(pos);
}

/**
 * The type of the electric field is set depending on the function used to apply it.
 */
FieldType Detector::getElectricFieldType() const {
    return electric_field_.getType();
}

/**
 * @throws std::invalid_argument If the electric field sizes are incorrect or the thickness domain is outside the sensor
 */
void Detector::setElectricFieldGrid(std::shared_ptr<std::vector<double>> field,
                                    std::array<size_t, 3> sizes,
                                    std::array<double, 2> scales,
                                    std::array<double, 2> offset,
                                    std::pair<double, double> thickness_domain) {
    electric_field_.setGrid(std::move(field), sizes, scales, offset, thickness_domain);
}

void Detector::setElectricFieldFunction(FieldFunction<ROOT::Math::XYZVector> function,
                                        std::pair<double, double> thickness_domain,
                                        FieldType type) {
    electric_field_.setFunction(std::move(function), thickness_domain, type);
}

bool Detector::hasWeightingField() const {
    return weighting_field_function_ ||
           (weighting_field_sizes_[0] != 0 && weighting_field_sizes_[1] != 0 && weighting_field_sizes_[2] != 0);
}

/**
 * The weighting field is retrieved relative to a reference pixel. Outside of the sensor the weighting field is strictly zero
 * by definition.
 */
ROOT::Math::XYZVector Detector::getWeightingField(const ROOT::Math::XYZPoint& pos, unsigned int px, unsigned int py) const {
    // FIXME: We need to revisit this to be faster and not too specific
    if(weighting_field_type_ == WeightingFieldType::NONE) {
        return ROOT::Math::XYZVector(0, 0, 0);
    }

    auto x = pos.x();
    auto y = pos.y();
    auto z = pos.z();

    // Compute distance from reference pixel:
    // WARNING This relies on the origin of the local coordinate system
    x -= px * model_->getPixelSize().x();
    y -= py * model_->getPixelSize().y();

    // Compute using the grid or a function depending on the setting
    ROOT::Math::XYZVector ret_val;
    if(weighting_field_type_ == WeightingFieldType::GRID) {
        // If outside the extent of the given weighting field, return zero:
        if(std::fabs(x) > weighting_field_extent_[0] / 2 || std::fabs(y) > weighting_field_extent_[1] / 2) {
            return ROOT::Math::XYZVector(0, 0, 0);
        }

        // The field is centered around the reference pixel, so we need to translate its coordinates:
        auto x_ind = static_cast<int>(std::floor(static_cast<double>(weighting_field_sizes_[0]) *
                                                 (x + weighting_field_extent_[0] / 2) / weighting_field_extent_[0]));
        auto y_ind = static_cast<int>(std::floor(static_cast<double>(weighting_field_sizes_[1]) *
                                                 (y + weighting_field_extent_[1] / 2.0) / weighting_field_extent_[1]));
        auto z_ind = static_cast<int>(
            std::floor(static_cast<double>(weighting_field_sizes_[2]) * (z - weighting_field_thickness_domain_.first) /
                       (weighting_field_thickness_domain_.second - weighting_field_thickness_domain_.first)));

        // Check for indices within the field
        // FIXME this is sort of redundant as we check the position above already. Serves as validity check for the indices
        // only
        if(x_ind < 0 || x_ind >= static_cast<int>(weighting_field_sizes_[0]) || y_ind < 0 ||
           y_ind >= static_cast<int>(weighting_field_sizes_[1]) || z_ind < 0 ||
           z_ind >= static_cast<int>(weighting_field_sizes_[2])) {
            return ROOT::Math::XYZVector(0, 0, 0);
        }

        // Compute total index
        size_t tot_ind = static_cast<size_t>(x_ind) * weighting_field_sizes_[1] * weighting_field_sizes_[2] * 3 +
                         static_cast<size_t>(y_ind) * weighting_field_sizes_[2] * 3 + static_cast<size_t>(z_ind) * 3;

        ret_val = ROOT::Math::XYZVector(
            (*electric_field_)[tot_ind], (*electric_field_)[tot_ind + 1], (*electric_field_)[tot_ind + 2]);
    } else {
        // Check if inside the thickness domain
        if(z < weighting_field_thickness_domain_.first || weighting_field_thickness_domain_.second < z) {
            return ROOT::Math::XYZVector(0, 0, 0);
        }

        // Calculate the weighting field
        ret_val = weighting_field_function_(ROOT::Math::XYZPoint(x, y, z));
    }

    return ret_val;
}

/**
 * The type of the weighting field is set depending on the function used to apply it.
 */
WeightingFieldType Detector::getWeightingFieldType() const {
    if(!hasWeightingField()) {
        return WeightingFieldType::NONE;
    }
    return weighting_field_type_;
}

/**
 * @throws std::invalid_argument If the weighting field sizes are incorrect or the thickness domain is outside the sensor
 *
 * The weighting field is stored as a large flat array. If the sizes are denoted as respectively X_SIZE, Y_ SIZE and Z_SIZE,
 * each position (x, y, z) has three indices:
 * - x*Y_SIZE*Z_SIZE*3+y*Z_SIZE*3+z*3: the x-component of the weighting field
 * - x*Y_SIZE*Z_SIZE*3+y*Z_SIZE*3+z*3+1: the y-component of the weighting field
 * - x*Y_SIZE*Z_SIZE*3+y*Z_SIZE*3+z*3+2: the z-component of the weighting field
 */
void Detector::setWeightingFieldGrid(std::shared_ptr<std::vector<double>> field,
                                     std::array<size_t, 3> sizes,
                                     std::array<double, 3> extent,
                                     std::pair<double, double> thickness_domain) {
    if(sizes[0] * sizes[1] * sizes[2] * 3 != field->size()) {
        throw std::invalid_argument("weighting field does not match the given sizes");
    }
    if(thickness_domain.first + 1e-9 < model_->getSensorCenter().z() - model_->getSensorSize().z() / 2.0 ||
       model_->getSensorCenter().z() + model_->getSensorSize().z() / 2.0 < thickness_domain.second - 1e-9) {
        throw std::invalid_argument("thickness domain is outside sensor dimensions");
    }
    if(thickness_domain.first >= thickness_domain.second) {
        throw std::invalid_argument("end of thickness domain is before begin");
    }

    weighting_field_ = std::move(field);
    weighting_field_extent_ = extent;
    weighting_field_sizes_ = sizes;
    weighting_field_thickness_domain_ = std::move(thickness_domain);
    weighting_field_type_ = WeightingFieldType::GRID;
}

void Detector::setWeightingFieldFunction(ElectricFieldFunction function,
                                         std::pair<double, double> thickness_domain,
                                         WeightingFieldType type) {
    weighting_field_thickness_domain_ = std::move(thickness_domain);
    weighting_field_function_ = std::move(function);
    weighting_field_type_ = type;
}

bool Detector::hasMagneticField() const {
    return magnetic_field_on_;
}

// TODO Currently the magnetic field in the detector is fixed to the field vector at it's center position. Change in case a
// field gradient is needed inside the sensor.
void Detector::setMagneticField(ROOT::Math::XYZVector b_field) {
    magnetic_field_on_ = true;
    magnetic_field_ = std::move(b_field);
}

/**
 * The magnetic field is evaluated for any sensor position.
 */
ROOT::Math::XYZVector Detector::getMagneticField() const {
    return magnetic_field_;
}
