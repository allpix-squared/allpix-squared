/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include <string>
#include <tuple>

#include "Detector.hpp"

using namespace allpix;

Detector::Detector(std::string name, std::shared_ptr<DetectorModel> model):
    name_(std::move(name)), model_(std::move(model)), location_(), orientation_(), external_models_() {}

// Set and get name of detector
std::string Detector::getName() const {
    return name_;
}

// Get the type of the model
std::string Detector::getType() const {
    return model_->getType();
}

// FIXME: implement
std::tuple<double, double, double> Detector::getPosition() const {
    return std::tuple<double, double, double>();
}

std::tuple<double, double, double> Detector::getOrientation() const {
    return std::tuple<double, double, double>();
}

// Return the model
const std::shared_ptr<DetectorModel> Detector::getModel() const {
    return model_;
}
