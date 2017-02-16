/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

// FIXME: this can be merged with the geometry manager I think (but keep separated for now just like the others)

#ifndef ALLPIX_DEFAULT_GEOMETRY_MANAGER_H
#define ALLPIX_DEFAULT_GEOMETRY_MANAGER_H

#include <vector>

#include "Detector.hpp"
#include "GeometryManager.hpp"

namespace allpix{

    class DefaultGeometryManager : public GeometryManager {
    public:
        // FIXME: not implemented
        virtual void setInternalDescription(std::string, void *) {};
        virtual void *getInternalDescription(std::string) {return nullptr; };
        
        // Add detector to the system
        virtual void addDetector(Detector);
        
        // Get detectors
        virtual std::vector<Detector> getDetectors() const;
        virtual Detector getDetector(std::string) const;
        virtual std::vector<Detector>  getDetectorsByType(std::string) const;
    private:
        std::vector<Detector> detectors_;
    };
}

#endif // ALLPIX_DEFAULT_GEOMETRY_MANAGER_H
