/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */
#ifndef ALLPIX_GEOMETRY_DETECTOR_MODEL_H
#define ALLPIX_GEOMETRY_DETECTOR_MODEL_H

#include <string>
#include <utility>

#include <Math/Point3D.h>
#include <Math/Vector2D.h>
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
        ROOT::Math::XYZVector getSensorSize() const { return sensor_size_; }
        double getSensorSizeX() const { return sensor_size_.x(); };
        double getSensorSizeY() const { return sensor_size_.y(); };
        double getSensorSizeZ() const { return sensor_size_.z(); };
        void setSensorSize(ROOT::Math::XYZVector val) { sensor_size_ = std::move(val); }

        // number of pixels (in general sense only copied units) in this detector
        // NOTE: default to 1, no copied units
        virtual ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> getNPixels() const {
            return ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>>(1, 1);
        }
        virtual int getNPixelsX() const { return 1; };
        virtual int getNPixelsY() const { return 1; };

        // size of a single pixel
        // default to the sensor size divided the amount of pixels
        virtual ROOT::Math::XYVector getPixelSize() const {
            return ROOT::Math::XYVector(getSensorSizeX() / getNPixelsX(), getSensorSizeY() / getNPixelsY());
        }
        virtual double getPixelSizeX() const { return getPixelSize().x(); };
        virtual double getPixelSizeY() const { return getPixelSize().y(); };

        // minimum and maximum coordinates of sensor in local frame
        // FIXME: is this a good way to implement this
        virtual double getSensorMinX() const = 0;
        virtual double getSensorMinY() const = 0;
        virtual double getSensorMinZ() const = 0;

        // coordinates of the rotation center in local frame
        // FIXME: it is a bit counter intuitive that this is not (0, 0, 0)
        virtual ROOT::Math::XYZPoint getCenter() const = 0;

    private:
        std::string type_;

        ROOT::Math::XYZVector sensor_size_;
    };
} // namespace allpix

#endif // ALLPIX_GEOMETRY_MANAGER_H
