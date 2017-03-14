/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include <string>
#include <tuple>

#include "DetectorModel.hpp"

using namespace allpix;

std::string DetectorModel::getType() const { return type_; }

void DetectorModel::setType(std::string type) { type_ = std::move(type); }
