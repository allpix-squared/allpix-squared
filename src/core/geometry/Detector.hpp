/**
 * @file
 * @brief Base of detector implementation
 *
 * @copyright MIT License
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
#include <Math/Point3D.h>
#include <Math/Transform3D.h>

#include "Detector.hpp"
#include "DetectorModel.hpp"

namespace allpix {

    /**
     * @brief Instantiation of a detector model in the world
     *
     * Contains the detector in the world with several unique properties (like the electric field). All model specific
     * properties are stored in its DetectorModel instead.
     */
    class Detector {
        friend class GeometryManager;

    public:
        /**
         * @brief Constructs a detector in the geometry
         * @param name Unique name of the detector
         * @param model Model of the detector
         * @param position Position in the world frame
         * @param orientation Orientation in Z-X-Z extrinsic Euler angles
         */
        Detector(std::string name,
                 std::shared_ptr<DetectorModel> model,
                 ROOT::Math::XYZPoint position,
                 ROOT::Math::EulerAngles orientation);

        /**
         * @brief Get name of the detector
         * @return Detector name
         */
        std::string getName() const;

        /**
         * @brief Get type of the detector
         * @return Type of the detector model
         */
        std::string getType() const;

        /**
         * @brief Get position in the world
         * @return Global position in Cartesian coordinates
         */
        ROOT::Math::XYZPoint getPosition() const;
        /**
         * @brief Get orientation in the world
         * @return Orientation in Z-X-Z extrinsic Euler angles
         */
        ROOT::Math::EulerAngles getOrientation() const;

        /**
         * @brief Convert a global position to a position in the detector frame
         * @param global_pos Position in the local frame
         * @return Position in the local frame
         */
        ROOT::Math::XYZPoint getLocalPosition(const ROOT::Math::XYZPoint& global_pos) const;
        /**
         * @brief Convert a global position to a position in the detector frame
         * @param local_pos Position in the local frame
         * @return Position in the global frame
         */
        ROOT::Math::XYZPoint getGlobalPosition(const ROOT::Math::XYZPoint& local_pos) const;

        /**
         * @brief Returns if a local position is within the sensitive device
         * @return True if a local position is within the sensor, false otherwise
         */
        bool isWithinSensor(const ROOT::Math::XYZPoint& local_pos) const;

        /**
         * @brief Returns if the detector has an electric field in the sensor
         * @return True if the detector has an electric field, false otherwise
         */
        bool hasElectricField() const;
        /**
         * @brief Get the electric field in the sensor at a local position
         * @param pos Position in the local frame
         * @return Vector of the field at the queried point
         */
        ROOT::Math::XYZVector getElectricField(const ROOT::Math::XYZPoint& local_pos) const;
        /**
         * @brief Get the raw electric field array at a local position
         * @param pos Position in a vector type having x(), y() and z() members
         * @return Pointer to an array containing the x, y, z components of the field
         *         (or a null pointer outside the sensor)
         */
        template <typename T> double* getElectricFieldRaw(T) const;

        /**
         * @brief Set the electric field in a single pixel in the detector
         * @param field Flat array of the field vectors (see detailed description)
         * @param sizes The dimensions of the flat electric field array
         */
        void setElectricField(std::shared_ptr<std::vector<double>> field, std::array<size_t, 3> sizes);

        /**
         * @brief Get the model of this detector
         * @return Pointer to the constant detector model
         */
        const std::shared_ptr<DetectorModel> getModel() const;

        /**
         * @brief Get an external geometry model by its type
         * @return External model
         */
        // TODO [doc] This feature should be removed
        template <typename T> std::shared_ptr<T> getExternalModel();
        /**
         * @brief Sets an external geometry model by its type
         * @param model External model
         */
        // TODO [doc] This feature should be removed
        template <typename T> void setExternalModel(std::shared_ptr<T> model);

    private:
        /**
         * @brief Constructs a detector in the geometry without a model (added later by the \ref GeometryManager)
         * @param name Unique name of the detector
         * @param position Position in the world frame
         * @param orientation Orientation in Z-X-Z extrinsic Euler angles
         */
        Detector(std::string name, ROOT::Math::XYZPoint position, ROOT::Math::EulerAngles orientation);

        /**
         * @brief Get the electric field at a position
         * @param x X-coordinate
         * @param y Y-coordinate
         * @param z Z-coordinate
         * @return Pointer to the electric field (or a null pointer if outside of the detector)
         */
        double* get_electric_field_raw(double x, double y, double z) const;

        /**
         * @brief Set the detector model (used by the \ref GeometryManager for lazy loading)
         * @param model Detector model to attach
         */
        void set_model(std::shared_ptr<DetectorModel> model);

        /**
         * @brief Create the coordinate transformation
         */
        void build_transform();

        std::string name_;
        std::shared_ptr<DetectorModel> model_;

        ROOT::Math::XYZPoint position_;
        ROOT::Math::EulerAngles orientation_;

        // Transform matrix from global to local coordinates
        ROOT::Math::Transform3D transform_;

        std::array<size_t, 3> electric_field_sizes_;
        std::shared_ptr<std::vector<double>> electric_field_;

        std::map<std::type_index, std::shared_ptr<void>> external_models_;
    };

    template <typename T> double* Detector::getElectricFieldRaw(T pos) const {
        return get_electric_field_raw(pos.x(), pos.y(), pos.z());
    }

    template <typename T> std::shared_ptr<T> Detector::getExternalModel() {
        return std::static_pointer_cast<T>(external_models_[typeid(T)]);
    }
    template <typename T> void Detector::setExternalModel(std::shared_ptr<T> model) {
        external_models_[typeid(T)] = std::static_pointer_cast<void>(model);
    }
} // namespace allpix

#endif /* ALLPIX_DETECTOR_H */
