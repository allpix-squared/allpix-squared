/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include <memory>
#include <string>
#include <utility>

#include "Detector.hpp"

using namespace allpix;

Detector::Detector(std::string name,
                   std::shared_ptr<DetectorModel> model,
                   ROOT::Math::XYZVector position,
                   ROOT::Math::EulerAngles orientation)
    : name_(std::move(name)), model_(std::move(model)), position_(std::move(position)), orientation_(orientation),
      external_models_() {}
Detector::Detector(std::string name, std::shared_ptr<DetectorModel> model)
    : Detector(std::move(name), std::move(model), ROOT::Math::XYZVector(), ROOT::Math::EulerAngles()) {}

// Set and get name of detector
std::string Detector::getName() const {
    return name_;
}

// Get the type of the model
std::string Detector::getType() const {
    return model_->getType();
}

// FIXME: implement
ROOT::Math::XYZVector Detector::getPosition() const {
    return position_;
}

ROOT::Math::EulerAngles Detector::getOrientation() const {
    return orientation_;
}

// Return the model
const std::shared_ptr<DetectorModel> Detector::getModel() const {
    return model_;
}
