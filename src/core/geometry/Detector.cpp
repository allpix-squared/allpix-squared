/**
 * @file
 * @brief Implementation of detector
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
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
    model_ = std::move(model); // NOLINT
    // Check if valid model is supplied
    if(model_ == nullptr) {
        throw InvalidModuleActionException("Detector model cannot be a null pointer");
    }

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
    : name_(std::move(name)), position_(std::move(position)), orientation_(orientation), magnetic_field_on_(false) {}

void Detector::set_model(std::shared_ptr<DetectorModel> model) {
    model_ = std::move(model);

    // Initialize the detector fields with the model:
    electric_field_.set_model(model_);
    weighting_potential_.set_model(model_);
    doping_profile_.set_model(model_);

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
    ROOT::Math::Translation3D translation_local(static_cast<ROOT::Math::XYZVector>(model_->getMatrixCenter()));
    ROOT::Math::Transform3D transform_local(translation_local);
    // Compute total transform local to global by first transforming local to locally centered and then to global coordinates
    transform_ = transform_center * transform_local.Inverse();
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
 * The pixel has internal information about the size and location specific for this detector
 */
Pixel Detector::getPixel(int x, int y) const {
    Pixel::Index index(x, y);
    return getPixel(index);
}

/**
 * The pixel has internal information about the size and location specific for this detector
 */
Pixel Detector::getPixel(const Pixel::Index& index) const {
    auto size = model_->getPixelSize();
    auto type = model_->getPixelType();

    auto local_center = model_->getPixelCenter(index.x(), index.y());
    auto global_center = getGlobalPosition(local_center);

    return {index, type, std::move(local_center), std::move(global_center), std::move(size)};
}

/**
 * The electric field is replicated for all pixels and uses flipping at each boundary (side effects are not modeled in this
 * stage). Outside of the sensor the electric field is strictly zero by definition.
 */
ROOT::Math::XYZVector Detector::getElectricField(const ROOT::Math::XYZPoint& local_pos) const {
    return electric_field_.get(local_pos);
}

/**
 * @throws std::invalid_argument If the electric field dimensions are incorrect or the thickness domain is outside the sensor
 */
void Detector::setElectricFieldGrid(const std::shared_ptr<std::vector<double>>& field,
                                    std::array<size_t, 3> bins,
                                    std::array<double, 3> size,
                                    FieldMapping mapping,
                                    std::array<double, 2> scales,
                                    std::array<double, 2> offset,
                                    std::pair<double, double> thickness_domain) {
    check_field_match(size, mapping, scales, thickness_domain);
    electric_field_.setGrid(field, bins, size, mapping, scales, offset, thickness_domain);
}

void Detector::setElectricFieldFunction(FieldFunction<ROOT::Math::XYZVector> function,
                                        std::pair<double, double> thickness_domain,
                                        FieldType type) {
    electric_field_.setFunction(std::move(function), thickness_domain, type);
}

/**
 * The weighting potential is retrieved relative to a reference pixel. Outside of the sensor the weighting potential is
 * strictly zero by definition.
 */
double Detector::getWeightingPotential(const ROOT::Math::XYZPoint& local_pos, const Pixel::Index& reference) const {
    auto ref = static_cast<ROOT::Math::XYPoint>(model_->getPixelCenter(reference.x(), reference.y()));

    // Requiring to extrapolate the field along z because equilibrium means no change in weighting potential,
    // Without this, we would get large jumps close to the electrode once charge carriers cross the boundary.
    return weighting_potential_.getRelativeTo(local_pos, ref, true);
}

/**
 * @throws std::invalid_argument If the weighting potential dimensions are incorrect or the thickness domain is outside the
 * sensor
 */
void Detector::setWeightingPotentialGrid(const std::shared_ptr<std::vector<double>>& potential,
                                         std::array<size_t, 3> bins,
                                         std::array<double, 3> size,
                                         FieldMapping mapping,
                                         std::array<double, 2> scales,
                                         std::array<double, 2> offset,
                                         std::pair<double, double> thickness_domain) {
    check_field_match(size, mapping, scales, thickness_domain);
    weighting_potential_.setGrid(potential, bins, size, mapping, scales, offset, thickness_domain);
}

void Detector::setWeightingPotentialFunction(FieldFunction<double> function,
                                             std::pair<double, double> thickness_domain,
                                             FieldType type) {
    weighting_potential_.setFunction(std::move(function), thickness_domain, type);
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
ROOT::Math::XYZVector Detector::getMagneticField(const ROOT::Math::XYZPoint&) const { return magnetic_field_; }

/**
 * The doping profile is replicated for all pixels and uses flipping at each boundary (side effects are not modeled in this
 * stage). Outside of the sensor the doping profile is strictly zero by definition.
 */
double Detector::getDopingConcentration(const ROOT::Math::XYZPoint& pos) const {
    // Extrapolate doping profile if outside defined field:
    return doping_profile_.get(pos, true);
}

/**
 * @throws std::invalid_argument If the doping profile dimensions are incorrect
 *
 * The doping profile is stored as a large flat array. If the sizes are denoted as respectively X_SIZE, Y_ SIZE and Z_SIZE,
 * each position (x, y, z) has one index, calculated as x*Y_SIZE*Z_SIZE+y*Z_SIZE+z
 */
void Detector::setDopingProfileGrid(std::shared_ptr<std::vector<double>> field,
                                    std::array<size_t, 3> bins,
                                    std::array<double, 3> size,
                                    FieldMapping mapping,
                                    std::array<double, 2> scales,
                                    std::array<double, 2> offset,
                                    std::pair<double, double> thickness_domain) {
    check_field_match(size, mapping, scales, thickness_domain);
    doping_profile_.setGrid(std::move(field), bins, size, mapping, scales, offset, thickness_domain);
}

void Detector::setDopingProfileFunction(FieldFunction<double> function, FieldType type) {
    doping_profile_.setFunction(std::move(function),
                                {model_->getSensorCenter().z() - model_->getSensorSize().z() / 2,
                                 model_->getSensorCenter().z() + model_->getSensorSize().z() / 2},
                                type);
}

void Detector::check_field_match(std::array<double, 3> size,
                                 FieldMapping mapping,
                                 std::array<double, 2> field_scale,
                                 std::pair<double, double> thickness_domain) const {

    // Check field dimension in z versus the requested thickness domain:
    auto eff_thickness = thickness_domain.second - thickness_domain.first;
    if(std::fabs(size[2] - eff_thickness) > std::numeric_limits<double>::epsilon()) {
        LOG(WARNING) << "Thickness of field is " << Units::display(size[2], "um") << " but the requested field depth is "
                     << Units::display(eff_thickness, "um");
    }

    // FIXME this could be done properly in the detector models at some point:
    if(model_->getPixelType() != Pixel::Type::RECTANGLE) {
        LOG(INFO) << "Pixels of this detector are not rectangular, will not perform further field matching checks";
        return;
    }

    // Check that the total field size is n*pitch:
    auto scale_x =
        field_scale[0] * (mapping == FieldMapping::SENSOR || mapping == FieldMapping::PIXEL_FULL ||
                                  mapping == FieldMapping::PIXEL_FULL_INVERSE || mapping == FieldMapping::PIXEL_HALF_TOP ||
                                  mapping == FieldMapping::PIXEL_HALF_BOTTOM
                              ? 1.0
                              : 0.5);
    auto scale_y =
        field_scale[1] * (mapping == FieldMapping::SENSOR || mapping == FieldMapping::PIXEL_FULL ||
                                  mapping == FieldMapping::PIXEL_FULL_INVERSE || mapping == FieldMapping::PIXEL_HALF_LEFT ||
                                  mapping == FieldMapping::PIXEL_HALF_RIGHT
                              ? 1.0
                              : 0.5);
    if(std::fabs(std::remainder(size[0] / scale_x, model_->getPixelSize().x())) > std::numeric_limits<double>::epsilon() ||
       std::fabs(std::remainder(size[1] / scale_y, model_->getPixelSize().y())) > std::numeric_limits<double>::epsilon()) {
        LOG(WARNING) << "Field map size is (" << Units::display(size[0] / scale_x, {"um", "mm"}) << ","
                     << Units::display(size[1] / scale_y, {"um", "mm"}) << ") but expecting a multiple of the pixel pitch ("
                     << Units::display(model_->getPixelSize().x(), {"um", "mm"}) << ", "
                     << Units::display(model_->getPixelSize().y(), {"um", "mm"}) << ")" << std::endl
                     << "The area to which the field is applied can be changed using the field_scale parameter.";
    } else {
        LOG(INFO) << "Field map size is (" << Units::display(size[0] / scale_x, {"um", "mm"}) << ","
                  << Units::display(size[1] / scale_y, {"um", "mm"}) << "), matching detector model with pixel pitch ("
                  << Units::display(model_->getPixelSize().x(), {"um", "mm"}) << ", "
                  << Units::display(model_->getPixelSize().y(), {"um", "mm"}) << ")";
    }
}
