/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_GEOMETRY_DETECTOR_H
#define ALLPIX_GEOMETRY_DETECTOR_H

#include <string>

#include "DetectorModel.hpp"

namespace allpix{

    class Detector{
    public:
        // Constructor and destructors
        Detector();
        ~Detector();
        
        //TODO: setters have to be added
        
        // Get description
        std::string getName() const;
        
        // Get location in world
        // FIXME: use a proper vector here
        std::tuple<double, double, double> getPosition() const;
        std::tuple<double, double, double> getOrientation() const;
        
        // get model description
        const DetectorModel &getModel();
    private:
        std::string name_;
        
        //FIXME: use a proper vector here
        std::tuple<double, double, double> location_;
        std::tuple<double, double, double> orientation_;
        
        DetectorModel model_;
    };
}

#endif // ALLPIX_GEOMETRY_MANAGER_H
