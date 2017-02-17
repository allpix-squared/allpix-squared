/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

// FIXME: this can be merged with the geometry manager I think (but keep separated for now just like the others)

#include <vector>
#include <iostream>

#include "DefaultGeometryManager.hpp"

#include "../utils/exceptions.h"

using namespace allpix;

// Add detector to the system
void DefaultGeometryManager::addDetector(Detector det){
    detectors_.push_back(det);
}

// Get detectors
std::vector<Detector> DefaultGeometryManager::getDetectors() const{
    return detectors_;
}

//FIXME: this is not a very nice way to do this
Detector DefaultGeometryManager::getDetector(std::string name) const{    
    for(auto &detector : detectors_){
        if(detector.getName() == name) return detector;
    }
    throw allpix::Exception("No detector with name " + name);
}
std::vector<Detector> DefaultGeometryManager::getDetectorsByType(std::string type) const{
    std::vector<Detector> result;
    for(auto &detector : detectors_){
        if(detector.getType() == type) result.push_back(detector);
    }
    return result;
}
