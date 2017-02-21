/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include <string>
#include <tuple>

#include "Detector.hpp"

using namespace allpix;

Detector::Detector(std::string name, std::string type): name_(name), type_(type) {}

std::string Detector::getName() const {
    return name_;
}

std::string Detector::getType() const {
    return type_;
}

// FIXME: implement
std::tuple<double, double, double> Detector::getPosition() const {
    return std::tuple<double, double, double>();
}

std::tuple<double, double, double> Detector::getOrientation() const {
    return std::tuple<double, double, double>();
}
