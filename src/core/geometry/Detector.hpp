/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_DETECTOR_H
#define ALLPIX_DETECTOR_H

#include <string>
#include <tuple>
#include <memory>

#include "Detector.hpp"
#include "DetectorModel.hpp"

namespace allpix {

    class Detector {
    public:
        // Constructor and destructors
        Detector() {}  // FIXME: remove this constructor?
        Detector(std::string name, std::shared_ptr<DetectorModel> model);
        virtual ~Detector() {}
        
        // TODO: setters have to be added
        
        // Get description
        std::string getName() const;
        void setName(std::string name);
        
        // Get the type of the detector model
        std::string getType() const;
        
        // Get location in world
        // FIXME: use a proper vector here
        std::tuple<double, double, double> getPosition() const;
        std::tuple<double, double, double> getOrientation() const;
        
        // get model description
        const std::shared_ptr<DetectorModel> getModel() const;
    private:        
        // FIXME: use a proper vector here
        std::tuple<double, double, double> location_;
        std::tuple<double, double, double> orientation_;

        std::string name_;        
        std::shared_ptr<DetectorModel> model_;        
    };
}

#endif /* ALLPIX_DETECTOR_H */
