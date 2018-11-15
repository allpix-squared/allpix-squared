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
    electric_field_.setModelParameters(model_->getPixelSize(),
                                       {model_->getSensorCenter().z() - model_->getSensorSize().z() / 2,
                                        model_->getSensorCenter().z() + model_->getSensorSize().z() / 2});
    doping_profile_.setModelParameters(model_->getPixelSize(),
                                       {model_->getSensorCenter().z() - model_->getSensorSize().z() / 2,
                                        model_->getSensorCenter().z() + model_->getSensorSize().z() / 2});
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
    return ((local_pos.x() >= model_->getSensorCenter().x() - model_->getSensorSize().x() / 2.0) &&
            (local_pos.x() <= model_->getSensorCenter().x() + model_->getSensorSize().x() / 2.0) &&
            (local_pos.y() >= model_->getSensorCenter().y() - model_->getSensorSize().y() / 2.0) &&
            (local_pos.y() <= model_->getSensorCenter().y() + model_->getSensorSize().y() / 2.0) &&
            (local_pos.z() >= model_->getSensorCenter().z() - model_->getSensorSize().z() / 2.0) &&
            (local_pos.z() <= model_->getSensorCenter().z() + model_->getSensorSize().z() / 2.0));
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

/**
 * The doping profile is replicated for all pixels and uses flipping at each boundary (side effects are not modeled in this
 * stage). Outside of the sensor the doping profile is strictly zero by definition.
 */
bool Detector::hasDopingProfile() const {
    return doping_profile_.isValid();
}

/**
 * The doping profile is replicated for all pixels and uses flipping at each boundary (side effects are not modeled in this
 * stage). Outside of the sensor the doping profile is strictly zero by definition.
 */
double Detector::getDopingProfile(const ROOT::Math::XYZPoint& pos) const {
    return doping_profile_.get(pos);
}

/**
 * The type of the doping profile is set depending on the function used to apply it.
 */
FieldType Detector::getDopingProfileType() const {
    return doping_profile_.getType();
}

/**
 * @throws std::invalid_argument If the dpong profile sizes are incorrect
 *
 * The doping profile is stored as a large flat array. If the sizes are denoted as respectively X_SIZE, Y_ SIZE and Z_SIZE,
 * each position (x, y, z) has one index, calculated as x*Y_SIZE*Z_SIZE+y*Z_SIZE+z
 */
void Detector::setDopingProfileGrid(std::shared_ptr<std::vector<double>> field,
                                    std::array<size_t, 3> sizes,
                                    std::array<double, 2> scales,
                                    std::array<double, 2> offset) {
    doping_profile_.setGrid(std::move(field),
                            sizes,
                            scales,
                            offset,
                            {model_->getSensorCenter().z() - model_->getSensorSize().z() / 2.0,
                             model_->getSensorCenter().z() + model_->getSensorSize().z() / 2.0});
}

void Detector::setDopingProfileFunction(FieldFunction<double> function, FieldType type) {
    doping_profile_.setFunction(std::move(function),
                                {model_->getSensorCenter().z() - model_->getSensorSize().z() / 2,
                                 model_->getSensorCenter().z() + model_->getSensorSize().z() / 2},
                                type);
}
