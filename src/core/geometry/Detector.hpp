/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_DETECTOR_H
#define ALLPIX_DETECTOR_H

#include <array>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <typeindex>
#include <vector>

#include <Math/EulerAngles.h>
#include <Math/Vector3D.h>

#include "Detector.hpp"
#include "DetectorModel.hpp"

namespace allpix {

    class Detector {
    public:
        // Constructor and destructors
        Detector(std::string name,
                 std::shared_ptr<DetectorModel> model,
                 ROOT::Math::XYZVector position,
                 ROOT::Math::EulerAngles orientation);
        Detector(std::string name, std::shared_ptr<DetectorModel> model);
        virtual ~Detector() = default;

        // Get description
        std::string getName() const;

        // Get the type of the detector model
        std::string getType() const;

        // Get location in world
        ROOT::Math::XYZVector getPosition() const;
        ROOT::Math::EulerAngles getOrientation() const;

        // Get fields in detector
        ROOT::Math::XYZVector getElectricField(const ROOT::Math::XYZVector&);
        // FIXME: is that a good way to provide an electric field
        void setElectricField(std::shared_ptr<std::vector<double>> field, std::array<unsigned int, 3> sizes);

        // get model description
        const std::shared_ptr<DetectorModel> getModel() const;

        // get model in external geometry
        // FIXME: is this a good way to approach this
        // FIXME: change name here?
        template <typename T> std::shared_ptr<T> getExternalModel();
        template <typename T> void setExternalModel(std::shared_ptr<T>);

    private:
        // name and model
        std::string name_;
        std::shared_ptr<DetectorModel> model_;

        // position and orientation
        ROOT::Math::XYZVector position_;
        ROOT::Math::EulerAngles orientation_;

        // fields
        std::array<unsigned int, 3> electric_field_sizes_;
        std::shared_ptr<std::vector<double>> electric_field_;

        // external models (FIXME: we probably want to drop this feature...)
        std::map<std::type_index, std::shared_ptr<void>> external_models_;
    };

    template <typename T> std::shared_ptr<T> Detector::getExternalModel() {
        return std::static_pointer_cast<T>(external_models_[typeid(T)]);
    }
    template <typename T> void Detector::setExternalModel(std::shared_ptr<T> model) {
        external_models_[typeid(T)] = std::static_pointer_cast<void>(model);
    }
} // namespace allpix

#endif /* ALLPIX_DETECTOR_H */
