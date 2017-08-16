/**
 * @file
 * @brief Implementation of detector
 *
 * @copyright MIT License
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
    : name_(std::move(name)), position_(std::move(position)), orientation_(orientation), electric_field_sizes_{{0, 0, 0}},
      electric_field_(nullptr) {}
void Detector::set_model(std::shared_ptr<DetectorModel> model) {
    model_ = std::move(model);
    build_transform();
}
void Detector::build_transform() {
    // Transform from center to local coordinate
    ROOT::Math::Translation3D translation_center(static_cast<ROOT::Math::XYZVector>(position_));
    ROOT::Math::Rotation3D rotation_center(orientation_);
    ROOT::Math::Transform3D transform_center(rotation_center.Inverse(), translation_center);
    // Transform from global to center
    ROOT::Math::Translation3D translation_local(static_cast<ROOT::Math::XYZVector>(model_->getCenter()));
    ROOT::Math::Transform3D transform_local(translation_local);
    // Compute total transform
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

    return Pixel(index, local_center, global_center, size);
}

/**
 * The electric field is replicated for all pixels and uses flipping at each boundary (side effects are not modeled in this
 * stage). Outside of the sensor the electric field is strictly zero by definition.
 */
bool Detector::hasElectricField() const {
    return electric_field_function_ ||
           (electric_field_sizes_[0] != 0 && electric_field_sizes_[1] != 0 && electric_field_sizes_[2] != 0);
}

/**
 * The electric field is replicated for all pixels and uses flipping at each boundary (side effects are not modeled in this
 * stage). Outside of the sensor the electric field is strictly zero by definition.
 */
ROOT::Math::XYZVector Detector::getElectricField(const ROOT::Math::XYZPoint& pos) const {
    auto field = get_electric_field_raw(pos.x(), pos.y(), pos.z());

    if(field.empty()) {
        // FIXME: Determine what we should do if we have no external electric field...
        return ROOT::Math::XYZVector(0, 0, 0);
    }

    return ROOT::Math::XYZVector(field.at(0), field.at(1), field.at(2));
}

/**
 * The type of the electric field is set depending on the function used to apply it.
 */
ElectricFieldType Detector::getElectricFieldType() const {
    if(!hasElectricField()) {
        return ElectricFieldType::NONE;
    }
    return electric_field_type_;
}

/**
 * The local position is first converted to pixel coordinates. The stored electric field if the index is odd.
 */
std::vector<double> Detector::get_electric_field_raw(double x, double y, double z) const {
    // FIXME: We need to revisit this to be faster and not too specific
    if(electric_field_type_ == ElectricFieldType::NONE) {
        return std::vector<double>();
    }

    // Compute corresponding pixel coordinates
    // WARNING This relies on the origin of the local coordinate system
    auto pixel_x = static_cast<int>(std::round(x / model_->getPixelSize().x()));
    auto pixel_y = static_cast<int>(std::round(y / model_->getPixelSize().y()));

    // Convert to the pixel frame
    x -= pixel_x * model_->getPixelSize().x();
    y -= pixel_y * model_->getPixelSize().y();

    // Do flipping if necessary
    if((pixel_x % 2) == 1) {
        x *= -1;
    }
    if((pixel_y % 2) == 1) {
        y *= -1;
    }

    // Compute using the grid or a function depending on the setting
    std::vector<double> ret_val(3, 0);
    if(electric_field_type_ == ElectricFieldType::GRID) {
        // Compute indices
        auto x_ind = static_cast<int>(std::floor(static_cast<double>(electric_field_sizes_[0]) *
                                                 (x + model_->getPixelSize().x() / 2.0) / model_->getPixelSize().x()));
        auto y_ind = static_cast<int>(std::floor(static_cast<double>(electric_field_sizes_[1]) *
                                                 (y + model_->getPixelSize().y() / 2.0) / model_->getPixelSize().y()));
        auto z_ind = static_cast<int>(
            std::floor(static_cast<double>(electric_field_sizes_[2]) * (z - electric_field_thickness_domain_.first) /
                       (electric_field_thickness_domain_.second - electric_field_thickness_domain_.first)));

        // Check for indices within the sensor
        if(x_ind < 0 || x_ind >= static_cast<int>(electric_field_sizes_[0]) || y_ind < 0 ||
           y_ind >= static_cast<int>(electric_field_sizes_[1]) || z_ind < 0 ||
           z_ind >= static_cast<int>(electric_field_sizes_[2])) {
            return std::vector<double>();
        }

        // Compute total index
        size_t tot_ind = static_cast<size_t>(x_ind) * electric_field_sizes_[1] * electric_field_sizes_[2] * 3 +
                         static_cast<size_t>(y_ind) * electric_field_sizes_[2] * 3 + static_cast<size_t>(z_ind) * 3;

        ret_val.at(0) = (*electric_field_)[tot_ind];
        ret_val.at(1) = (*electric_field_)[tot_ind + 1];
        ret_val.at(2) = (*electric_field_)[tot_ind + 2];
    } else {
        // FIXME This is not efficient...
        // Check if inside the thickness domain
        if(z < electric_field_thickness_domain_.first || electric_field_thickness_domain_.second < z) {
            return ret_val;
        }

        // Calculate the electric field
        auto vector = electric_field_function_(ROOT::Math::XYZPoint(x, y, z));
        vector.GetCoordinates(ret_val.at(0), ret_val.at(1), ret_val.at(2));
    }

    // Flip vector if necessary
    if((pixel_x % 2) == 1) {
        ret_val.at(0) *= -1;
    }
    if((pixel_y % 2) == 1) {
        ret_val.at(1) *= -1;
    }

    return ret_val;
}

/**
 * @throws std::invalid_argument If the electric field sizes are incorrect or the thickness domain is outside the sensor
 *
 * The electric field is stored as a large flat array. If the sizes are denoted as respectively X_SIZE, Y_ SIZE and Z_SIZE,
 * each position (x, y, z) has three indices:
 * - x*Y_SIZE*Z_SIZE*3+y*Z_SIZE*3+z*3: the x-component of the electric field
 * - x*Y_SIZE*Z_SIZE*3+y*Z_SIZE*3+z*3+1: the y-component of the electric field
 * - x*Y_SIZE*Z_SIZE*3+y*Z_SIZE*3+z*3+2: the z-component of the electric field
 */
void Detector::setElectricFieldGrid(std::shared_ptr<std::vector<double>> field,
                                    std::array<size_t, 3> sizes,
                                    std::pair<double, double> thickness_domain) {
    if(sizes[0] * sizes[1] * sizes[2] * 3 != field->size()) {
        throw std::invalid_argument("electric field does not match the given sizes");
    }
    if(thickness_domain.first + 1e-9 < model_->getSensorCenter().z() - model_->getSensorSize().z() / 2.0 ||
       model_->getSensorCenter().z() + model_->getSensorSize().z() / 2.0 < thickness_domain.second - 1e-9) {
        throw std::invalid_argument("thickness domain is outside sensor dimensions");
    }
    if(thickness_domain.first >= thickness_domain.second) {
        throw std::invalid_argument("end of thickness domain is before begin");
    }

    electric_field_ = std::move(field);
    electric_field_sizes_ = sizes;
    electric_field_thickness_domain_ = std::move(thickness_domain);
    electric_field_type_ = ElectricFieldType::GRID;
}

void Detector::setElectricFieldFunction(ElectricFieldFunction function,
                                        std::pair<double, double> thickness_domain,
                                        ElectricFieldType type) {
    electric_field_thickness_domain_ = std::move(thickness_domain);
    electric_field_function_ = std::move(function);
    electric_field_type_ = type;
}
