/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_GEOMETRY_MANAGER_H
#define ALLPIX_GEOMETRY_MANAGER_H

#include <vector>

#include "Detector.hpp"

namespace allpix{

    class GeometryManager{
    public:
        // Constructor and destructors
        GeometryManager() {}
        virtual ~GeometryManager() {}
        
        // Disallow copy
        GeometryManager(const GeometryManager&) = delete;
        GeometryManager &operator=(const GeometryManager&) = delete;
        
        // Get internal representation
        // FIXME: this is not very elegant, but it is probably the only option (possible use a in-between type?)
        virtual void setInternalDescription(std::string, void *) = 0;
        virtual void *getInternalDescription(std::string) = 0;
        
        // Add detector to the system
        virtual void addDetector(Detector) = 0;
        
        // Get detectors
        virtual std::vector<Detector> getDetectors() const = 0;
        virtual Detector getDetector(std::string) const = 0;
        virtual std::vector<Detector>  getDetectorsByType(std::string) const = 0;
    };
}

#endif // ALLPIX_GEOMETRY_MANAGER_H
