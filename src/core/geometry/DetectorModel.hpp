/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */
#ifndef ALLPIX_GEOMETRY_DETECTOR_MODEL_H
#define ALLPIX_GEOMETRY_DETECTOR_MODEL_H

#include <string>
#include <utility>

#include <Math/Vector3D.h>

namespace allpix {
    class DetectorModel {
    public:
        // constructor and destructor
        explicit DetectorModel(std::string type);
        virtual ~DetectorModel();

        // name of detector model
        std::string getType() const;
        void setType(std::string type);

        // FIXME: what to provide as minimum here
        // sensor size of the model

        // sensor sizes
        ROOT::Math::XYZVector getSensorSize() { return sensor_size_; }
        double getSensorSizeX() { return sensor_size_.x(); };
        double getSensorSizeY() { return sensor_size_.y(); };
        double getSensorSizeZ() { return sensor_size_.z(); };
        void setSensorSize(ROOT::Math::XYZVector val) { sensor_size_ = std::move(val); }

        // minimum and maximum coordinates of sensor in local frame
        // FIXME: is this a good way to implement this
        virtual double getSensorMinX() = 0;
        virtual double getSensorMinY() = 0;
        virtual double getSensorMinZ() = 0;

    private:
        std::string type_;

        ROOT::Math::XYZVector sensor_size_;
    };
} // namespace allpix

#endif // ALLPIX_GEOMETRY_MANAGER_H
