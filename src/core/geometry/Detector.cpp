#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <Math/Rotation3D.h>
#include <Math/Translation3D.h>

#include "Detector.hpp"

using namespace allpix;

/**
 * @throws std::invalid_argument If the detector model pointer is a null pointer
 *
 * Creates a detector without any electric field in the sensor.
 */
Detector::Detector(std::string name,
                   std::shared_ptr<DetectorModel> model,
                   ROOT::Math::XYZPoint position,
                   ROOT::Math::EulerAngles orientation)
    : name_(std::move(name)), model_(std::move(model)), position_(std::move(position)), orientation_(orientation),
      transform_(), electric_field_sizes_{{0, 0, 0}}, electric_field_(nullptr), external_models_() {
    // Check if valid model is supplied
    if(model_ == nullptr) {
        throw std::invalid_argument("detector model cannot be null");
    }

    // Construct transform
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
ROOT::Math::EulerAngles Detector::getOrientation() const {
    return orientation_;
}

/**
 * @warning The local coordinate position does normally not have its origin at the center of rotation
 *
 * The origin of the local frame is at the center of the first pixel in the middle of the sensor
 */
ROOT::Math::XYZPoint Detector::getLocalPosition(const ROOT::Math::XYZPoint& global_pos) const {
    return transform_.Inverse()(global_pos);
}
ROOT::Math::XYZPoint Detector::getGlobalPosition(const ROOT::Math::XYZPoint& local_pos) const {
    return transform_(local_pos);
}

/**
 * The electric field is replicated for all pixels and uses flipping at each boundary (side effects are not modeled in this
 * stage). Outside of the sensor the electric field is strictly zero by definition.
 */
ROOT::Math::XYZVector Detector::getElectricField(const ROOT::Math::XYZPoint& pos) const {
    double* field = get_electric_field_raw(pos.x(), pos.y(), pos.z());

    if(field == nullptr) {
        // FIXME: Determine what we should do if we have no external electric field...
        return ROOT::Math::XYZVector(0, 0, 0);
    }

    return ROOT::Math::XYZVector(*(field), *(field + 1), *(field + 2));
}
/**
 * The local position is first converted to pixel coordinates. The stored electric field if the index is odd.
 */
double* Detector::get_electric_field_raw(double x, double y, double z) const {
    // FIXME: We need to revisit this to be faster and not too specific

    // Compute corresponding pixel indices
    auto pixel_x = static_cast<int>(std::round(x / model_->getPixelSizeX()));
    auto pixel_y = static_cast<int>(std::round(y / model_->getPixelSizeY()));

    // Convert to the pixel frame
    x -= pixel_x * model_->getPixelSizeX();
    y -= pixel_y * model_->getPixelSizeY();

    // Do flipping if necessary
    if((pixel_x % 2) == 1) {
        x *= -1;
    }
    if((pixel_y % 2) == 1) {
        y *= -1;
    }

    // Compute indices
    auto x_ind = static_cast<int>(static_cast<double>(electric_field_sizes_[0]) * (x + model_->getPixelSizeX() / 2.0) /
                                  model_->getPixelSizeX());
    auto y_ind = static_cast<int>(static_cast<double>(electric_field_sizes_[1]) * (y + model_->getPixelSizeY() / 2.0) /
                                  model_->getPixelSizeY());
    auto z_ind = static_cast<int>(static_cast<double>(electric_field_sizes_[2]) * (z - model_->getSensorMinZ()) /
                                  model_->getSensorSizeZ());

    // Check for indices within the sensor
    if(x_ind < 0 || x_ind >= static_cast<int>(electric_field_sizes_[0]) || y_ind < 0 ||
       y_ind >= static_cast<int>(electric_field_sizes_[1]) || z_ind < 0 ||
       z_ind >= static_cast<int>(electric_field_sizes_[2])) {
        return nullptr;
    }

    // Compute total index
    size_t tot_ind = static_cast<size_t>(x_ind) * electric_field_sizes_[1] * electric_field_sizes_[2] * 3 +
                     static_cast<size_t>(y_ind) * electric_field_sizes_[2] * 3 + static_cast<size_t>(z_ind) * 3;
    return &(*electric_field_)[tot_ind];
}

/**
 * The electric field is stored as a large flat array. If the sizes are denoted as respectively X_SIZE, Y_ SIZE and Z_SIZE,
 * each position (x, y, z) has three indices:
 * - x*Y_SIZE*Z_SIZE*3+y*Z_SIZE*3+z*3: the x-component of the electric field
 * - x*Y_SIZE*Z_SIZE*3+y*Z_SIZE*3+z*3+1: the y-component of the electric field
 * - x*Y_SIZE*Z_SIZE*3+y*Z_SIZE*3+z*3+2: the z-component of the electric field
 */
void Detector::setElectricField(std::shared_ptr<std::vector<double>> field, std::array<size_t, 3> sizes) {
    if(sizes[0] * sizes[1] * sizes[2] * 3 != field->size()) {
        throw std::invalid_argument("electric field does not match the given sizes");
    }
    electric_field_ = std::move(field);
    electric_field_sizes_ = sizes;
}
