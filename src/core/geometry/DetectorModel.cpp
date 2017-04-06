/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include <string>
#include <utility>

#include "DetectorModel.hpp"

using namespace allpix;

// constructor and destructor
DetectorModel::DetectorModel(std::string type) : type_(std::move(type)), sensor_size_() {}
DetectorModel::~DetectorModel() = default;

std::string DetectorModel::getType() const {
    return type_;
}

void DetectorModel::setType(std::string type) {
    type_ = std::move(type);
}
