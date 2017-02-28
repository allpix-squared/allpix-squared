/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include <string>
#include <tuple>

#include "Detector.hpp"

using namespace allpix;

Detector::Detector(std::string name, std::shared_ptr<DetectorModel> model): name_(name), model_(model) {}

// Set and get name of detector
std::string Detector::getName() const {
    return name_;
}
/*void Detector::setName(std::string name) {
    name_ = name;
}*/

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
