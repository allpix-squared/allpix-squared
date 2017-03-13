/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_DETECTOR_H
#define ALLPIX_DETECTOR_H

#include <string>
#include <tuple>
#include <memory>
#include <typeindex>
#include <map>

#include "Detector.hpp"
#include "DetectorModel.hpp"

namespace allpix {

    class Detector {
    public:
        // Constructor and destructors
        Detector(std::string name, std::shared_ptr<DetectorModel> model);
        virtual ~Detector() = default;

        // Get description
        std::string getName() const;
        //FIXME: name is fixed after creation
        // void setName(std::string name);

        // Get the type of the detector model
        std::string getType() const;

        // Get location in world
        // FIXME: use a proper vector here
        std::tuple<double, double, double> getPosition() const;
        std::tuple<double, double, double> getOrientation() const;

        // get model description
        const std::shared_ptr<DetectorModel> getModel() const;

        // get model in external geometry
        // FIXME: is this a good way to approach this
        // FIXME: change name here?
        template<typename T> std::shared_ptr<T> getExternalModel();
        template<typename T> void setExternalModel(std::shared_ptr<T>);
    private:
        std::string name_;
        std::shared_ptr<DetectorModel> model_;

        // FIXME: use a proper vector here
        std::tuple<double, double, double> location_;
        std::tuple<double, double, double> orientation_;

        std::map<std::type_index, std::shared_ptr<void>> external_models_;
    };

    template<typename T> std::shared_ptr<T> Detector::getExternalModel(){
        return std::static_pointer_cast<T>(external_models_[typeid(T)]);
    }
    template<typename T> void Detector::setExternalModel(std::shared_ptr<T> model){
        external_models_[typeid(T)] = std::static_pointer_cast<void>(model);
    }
}

#endif /* ALLPIX_DETECTOR_H */
