/**
 * @file
 * @brief Implementation of geometry manager
 *
 * @copyright MIT License
 */

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "GeometryManager.hpp"
#include "core/module/exceptions.h"
#include "core/utils/log.h"
#include "exceptions.h"

using namespace allpix;

GeometryManager::GeometryManager() = default;

/**
 * @throws InvalidModuleActionException If the passed detector is a null pointer
 * @throws DetectorNameExistsError If the detector name is already registered before
 */
void GeometryManager::addDetector(std::shared_ptr<Detector> detector) {
    if(detector == nullptr) {
        throw InvalidModuleActionException("Detector to add cannot be a null pointer");
    }

    LOG(TRACE) << "Registering new detector " << detector->getName();
    if(detector_names_.find(detector->getName()) != detector_names_.end()) {
        throw DetectorNameExistsError(detector->getName());
    }

    detector_names_.insert(detector->getName());
    detectors_.push_back(std::move(detector));
}

std::vector<std::shared_ptr<Detector>> GeometryManager::getDetectors() const {
    return detectors_;
}

/**
 * @throws InvalidDetectorError If a detector with this name does not exist
 */
std::shared_ptr<Detector> GeometryManager::getDetector(const std::string& name) const {
    // FIXME: this is not a very nice way to implement this
    for(auto& detector : detectors_) {
        if(detector->getName() == name) {
            return detector;
        }
    }
    throw allpix::InvalidDetectorError("name", name);
}
/**
 * @throws InvalidDetectorError If not a single detector with this type exists
 */
std::vector<std::shared_ptr<Detector>> GeometryManager::getDetectorsByType(const std::string& type) const {
    std::vector<std::shared_ptr<Detector>> result;
    for(auto& detector : detectors_) {
        if(detector->getType() == type) {
            result.push_back(detector);
        }
    }
    if(result.empty()) {
        throw allpix::InvalidDetectorError("type", type);
    }

    return result;
}
