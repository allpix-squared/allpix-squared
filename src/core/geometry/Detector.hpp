/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_DETECTOR_H
#define ALLPIX_DETECTOR_H

#include <string>
#include <tuple>

#include "Detector.hpp"

namespace allpix {

    class Detector {
    public:
        // Constructor and destructors
        Detector() {}  // FIXME: remove this constructor?
        Detector(std::string name, std::string type);
        ~Detector() {}
        
        // TODO: setters have to be added
        
        // Get description
        std::string getName() const;
        std::string getType() const;
        
        // Get location in world
        // FIXME: use a proper vector here
        std::tuple<double, double, double> getPosition() const;
        std::tuple<double, double, double> getOrientation() const;
        
        // get model description
        // const DetectorModel &getModel();
    private:        
        // FIXME: use a proper vector here
        std::tuple<double, double, double> location_;
        std::tuple<double, double, double> orientation_;
        
        // DetectorModel model_;
        std::string name_;
        std::string type_;
    };
}

#endif /* ALLPIX_DETECTOR_H */
