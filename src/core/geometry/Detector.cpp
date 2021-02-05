/**
 * @file
 * @brief Implementation of detector
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
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

    // Initialize the detector fields with the model parameters:
    electric_field_.set_model_parameters(model_->getSensorCenter(), model_->getSensorSize(), model_->getPixelSize());
    weighting_potential_.set_model_parameters(model_->getSensorCenter(), model_->getSensorSize(), model_->getPixelSize());
    doping_profile_.set_model_parameters(model_->getSensorCenter(), model_->getSensorSize(), model_->getPixelSize());

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
 * The definition of the pixel grid size is determined by the detector model
 */
bool Detector::isWithinPixelGrid(const Pixel::Index& pixel_index) const {
    return !(pixel_index.x() >= model_->getNPixels().x() || pixel_index.y() >= model_->getNPixels().y());
}

/**
 * The definition of the pixel grid size is determined by the detector model
 */
bool Detector::isWithinPixelGrid(const int x, const int y) const {
    return !(x < 0 || x >= static_cast<int>(model_->getNPixels().x()) || y < 0 ||
             y >= static_cast<int>(model_->getNPixels().y()));
}

/**
 * The pixel has internal information about the size and location specific for this detector
 */
Pixel Detector::getPixel(unsigned int x, unsigned int y) const {
    Pixel::Index index(x, y);
    return getPixel(index);
}

/**
 * The pixel has internal information about the size and location specific for this detector
 */
Pixel Detector::getPixel(const Pixel::Index& index) const {
    auto size = model_->getPixelSize();

    // WARNING This relies on the origin of the local coordinate system
    auto local_x = size.x() * index.x();
    auto local_y = size.y() * index.y();
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
 * @throws std::invalid_argument If the electric field dimensions are incorrect or the thickness domain is outside the sensor
 */
void Detector::setElectricFieldGrid(const std::shared_ptr<std::vector<double>>& field,
                                    std::array<size_t, 3> dimensions,
                                    std::array<double, 2> scales,
                                    std::array<double, 2> offset,
                                    std::pair<double, double> thickness_domain) {
    electric_field_.setGrid(field, dimensions, scales, offset, thickness_domain);
}

void Detector::setElectricFieldFunction(FieldFunction<ROOT::Math::XYZVector> function,
                                        std::pair<double, double> thickness_domain,
                                        FieldType type) {
    electric_field_.setFunction(std::move(function), thickness_domain, type);
}

bool Detector::hasWeightingPotential() const {
    return weighting_potential_.isValid();
}

/**
 * The weighting potential is retrieved relative to a reference pixel. Outside of the sensor the weighting potential is
 * strictly zero by definition.
 */
double Detector::getWeightingPotential(const ROOT::Math::XYZPoint& pos, const Pixel::Index& reference) const {
    auto size = model_->getPixelSize();

    // WARNING This relies on the origin of the local coordinate system
    auto local_x = size.x() * reference.x();
    auto local_y = size.y() * reference.y();

    // Requiring to extrapolate the field along z because equilibrium means no change in weighting potential,
    // Without this, we would get large jumps close to the electrode once charge carriers cross the boundary.
    return weighting_potential_.getRelativeTo(pos, {local_x, local_y}, true);
}

/**
 * The type of the weighting potential is set depending on the function used to apply it.
 */
FieldType Detector::getWeightingPotentialType() const {
    return weighting_potential_.getType();
}

/**
 * @throws std::invalid_argument If the weighting potential dimensions are incorrect or the thickness domain is outside the
 * sensor
 */
void Detector::setWeightingPotentialGrid(const std::shared_ptr<std::vector<double>>& potential,
                                         std::array<size_t, 3> dimensions,
                                         std::array<double, 2> scales,
                                         std::array<double, 2> offset,
                                         std::pair<double, double> thickness_domain) {
    weighting_potential_.setGrid(potential, dimensions, scales, offset, thickness_domain);
}

void Detector::setWeightingPotentialFunction(FieldFunction<double> function,
                                             std::pair<double, double> thickness_domain,
                                             FieldType type) {
    weighting_potential_.setFunction(std::move(function), thickness_domain, type);
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
    // Extrapolate doping profile if outside defined field:
    return doping_profile_.get(pos, true);
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
                                    std::array<double, 2> offset,
                                    std::pair<double, double> thickness_domain) {
    doping_profile_.setGrid(std::move(field), sizes, scales, offset, thickness_domain);
}

void Detector::setDopingProfileFunction(FieldFunction<double> function, FieldType type) {
    doping_profile_.setFunction(std::move(function),
                                {model_->getSensorCenter().z() - model_->getSensorSize().z() / 2,
                                 model_->getSensorCenter().z() + model_->getSensorSize().z() / 2},
                                type);
}
