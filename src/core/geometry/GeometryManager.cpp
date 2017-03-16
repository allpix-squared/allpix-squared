/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

// FIXME: this can be merged with the geometry manager I think (but keep separated for now just like the others)

#include <string>
#include <vector>

#include "GeometryManager.hpp"

#include "core/utils/exceptions.h"

using namespace allpix;

// Constructor and destructor
GeometryManager::GeometryManager() : detectors_(), detector_names_() {}
GeometryManager::~GeometryManager() = default;

// Add detector to the system
void GeometryManager::addDetector(std::shared_ptr<Detector> det) {
    if(detector_names_.find(det->getName()) != detector_names_.end()) {
        throw DetectorNameExistsError(det->getName());
    }

    detector_names_.insert(det->getName());
    detectors_.push_back(std::move(det));
}

// Get detectors
std::vector<std::shared_ptr<Detector>> GeometryManager::getDetectors() const { return detectors_; }

// FIXME: this is not a very nice way to do this
std::shared_ptr<Detector> GeometryManager::getDetector(const std::string& name) const {
    for(auto& detector : detectors_) {
        if(detector->getName() == name) {
            return detector;
        }
    }
    throw allpix::InvalidDetectorError("name", name);
}
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
