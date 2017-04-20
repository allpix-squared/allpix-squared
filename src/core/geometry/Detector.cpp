/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <Math/Rotation3D.h>
#include <Math/Translation3D.h>

#include "Detector.hpp"

using namespace allpix;

Detector::Detector(std::string name,
                   std::shared_ptr<DetectorModel> model,
                   ROOT::Math::XYZPoint position,
                   ROOT::Math::EulerAngles orientation)
    : name_(std::move(name)), model_(std::move(model)), position_(std::move(position)), orientation_(orientation),
      transform_(), electric_field_sizes_{{0, 0, 0}}, electric_field_(nullptr), external_models_() {
    // check model
    if(model_ == nullptr) {
        throw std::invalid_argument("detector model cannot be null");
    }

    // build transforms
    // sensor midpoint transform
    ROOT::Math::Translation3D translation_center(static_cast<ROOT::Math::XYZVector>(position_));
    ROOT::Math::Rotation3D rotation_center(orientation_);
    ROOT::Math::Transform3D transform_center(rotation_center.Inverse(), translation_center);

    // sensor pixel transform
    ROOT::Math::Translation3D translation_local(static_cast<ROOT::Math::XYZVector>(model_->getCenter()));
    ROOT::Math::Transform3D transform_local(translation_local);

    // set total transform
    transform_ = transform_center * transform_local.Inverse();
}
Detector::Detector(std::string name, std::shared_ptr<DetectorModel> model)
    : Detector(std::move(name), std::move(model), ROOT::Math::XYZPoint(), ROOT::Math::EulerAngles()) {}

// Set and get name of detector
std::string Detector::getName() const {
    return name_;
}

// Get the type of the model
std::string Detector::getType() const {
    return model_->getType();
}

// Return the model
const std::shared_ptr<DetectorModel> Detector::getModel() const {
    return model_;
}

// Get position of detector
ROOT::Math::XYZPoint Detector::getPosition() const {
    return position_;
}

// Get orientation of model (FIXME: specify clearly which convention is used)
ROOT::Math::EulerAngles Detector::getOrientation() const {
    return orientation_;
}

// Convert between coordinates
// WARNING: (0, 0, 0) local coordinates is not the same as global position!
ROOT::Math::XYZPoint Detector::getLocalPosition(const ROOT::Math::XYZPoint& global_pos) const {
    return transform_.Inverse()(global_pos);
}
ROOT::Math::XYZPoint Detector::getGlobalPosition(const ROOT::Math::XYZPoint& local_pos) const {
    return transform_(local_pos);
}

// Get fields in detector
ROOT::Math::XYZVector Detector::getElectricField(const ROOT::Math::XYZPoint& pos) const {
    double* field = get_electric_field_raw(pos.x(), pos.y(), pos.z());

    if(field == nullptr) {
        // FIXME: determine what we should do if we have no external electric field...
        return ROOT::Math::XYZVector(0, 0, 0);
    }

    return ROOT::Math::XYZVector(*(field), *(field + 1), *(field + 2));
}
// Get fields in detector
double* Detector::get_electric_field_raw(double x, double y, double z) const {
    // compute indices
    // FIXME: can we calculate this faster using vector calculations
    int x_ind = static_cast<int>(static_cast<double>(electric_field_sizes_[0]) * (x - model_->getSensorMinX()) /
                                 model_->getSensorSizeX());
    int y_ind = static_cast<int>(static_cast<double>(electric_field_sizes_[1]) * (y - model_->getSensorMinY()) /
                                 model_->getSensorSizeY());
    int z_ind = static_cast<int>(static_cast<double>(electric_field_sizes_[2]) * (z - model_->getSensorMinZ()) /
                                 model_->getSensorSizeZ());

    // check for indices within the sensor
    if(x_ind < 0 || x_ind >= static_cast<int>(electric_field_sizes_[0]) || y_ind < 0 ||
       y_ind >= static_cast<int>(electric_field_sizes_[1]) || z_ind < 0 ||
       z_ind >= static_cast<int>(electric_field_sizes_[2])) {
        return nullptr;
    }

    size_t tot_ind = static_cast<size_t>(x_ind) * electric_field_sizes_[1] * electric_field_sizes_[2] * 3 +
                     static_cast<size_t>(y_ind) * electric_field_sizes_[2] * 3 + static_cast<size_t>(z_ind) * 3;
    return &(*electric_field_)[tot_ind];
}

// FIXME: is that a good way to provide an electric field
void Detector::setElectricField(std::shared_ptr<std::vector<double>> field, std::array<size_t, 3> sizes) {
    if(sizes[0] * sizes[1] * sizes[2] * 3 != field->size()) {
        throw std::invalid_argument("electric field does not match the given sizes");
    }
    electric_field_ = std::move(field);
    electric_field_sizes_ = sizes;
}
