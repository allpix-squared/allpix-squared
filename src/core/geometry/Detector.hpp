/**
 * @file
 * @brief Base of detector implementation
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_DETECTOR_H
#define ALLPIX_DETECTOR_H

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <typeindex>
#include <vector>

#include <Math/Point3D.h>
#include <Math/Rotation3D.h>
#include <Math/Transform3D.h>

#include "Detector.hpp"
#include "DetectorField.hpp"
#include "DetectorModel.hpp"

#include "objects/Pixel.hpp"

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
         * @param orientation Rotation matrix representing the orientation
         */
        Detector(std::string name,
                 std::shared_ptr<DetectorModel> model,
                 ROOT::Math::XYZPoint position,
                 const ROOT::Math::Rotation3D& orientation);

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
         * @return Rotation matrix representing the orientation
         */
        ROOT::Math::Rotation3D getOrientation() const;

        /**
         * @brief Convert a global position to a position in the detector frame
         * @param global_pos Position in the global frame
         * @return Position in the local frame
         */
        ROOT::Math::XYZPoint getLocalPosition(const ROOT::Math::XYZPoint& global_pos) const;
        /**
         * @brief Convert a position in the detector frame to a global position
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
         * @brief Returns if a local position is within the pixel implant region of the sensitive device
         * @return True if a local position is within the pixel implant, false otherwise
         */
        bool isWithinImplant(const ROOT::Math::XYZPoint& local_pos) const;

        /**
         * @brief Returns if a pixel index is within the grid of pixels defined for the device
         * @return True if pixel_index is within the pixel grid, false otherwise
         */
        bool isWithinPixelGrid(const Pixel::Index& pixel_index) const;

        /**
         * @brief Returns if a set of pixel coordinates is within the grid of pixels defined for the device
         * @return True if pixel coordinates are within the pixel grid, false otherwise
         */
        bool isWithinPixelGrid(const int x, const int y) const;

        /**
         * @brief Return a pixel object from the x- and y-index values
         * @return Pixel object
         */
        Pixel getPixel(unsigned int x, unsigned int y) const;

        /**
         * @brief Return a pixel object from the pixel index
         * @return Pixel object
         */
        Pixel getPixel(const Pixel::Index& index) const;

        /**
         * @brief Returns if the detector has an electric field in the sensor
         * @return True if the detector has an electric field, false otherwise
         */
        bool hasElectricField() const;
        /**
         * @brief Return the type of electric field that is simulated.
         * @return The type of the electric field
         */
        FieldType getElectricFieldType() const;
        /**
         * @brief Get the electric field in the sensor at a local position
         * @param pos Position in the local frame
         * @return Vector of the field at the queried point
         */
        ROOT::Math::XYZVector getElectricField(const ROOT::Math::XYZPoint& local_pos) const;

        /**
         * @brief Set the electric field in a single pixel in the detector using a grid
         * @param field Flat array of the field vectors (see detailed description)
         * @param sizes The dimensions of the flat electric field array
         * @param scales Scaling factors for the field size, given in fractions of a pixel unit cell in x and y
         * @param thickness_domain Domain in local coordinates in the thickness direction where the field holds
         */
        void setElectricFieldGrid(const std::shared_ptr<std::vector<double>>& field,
                                  std::array<size_t, 3> sizes,
                                  std::array<double, 2> scales,
                                  std::array<double, 2> offset,
                                  std::pair<double, double> thickness_domain);
        /**
         * @brief Set the electric field in a single pixel using a function
         * @param function Function used to retrieve the electric field
         * @param type Type of the electric field function used
         * @param thickness_domain Domain in local coordinates in the thickness direction where the field holds
         */
        void setElectricFieldFunction(FieldFunction<ROOT::Math::XYZVector> function,
                                      std::pair<double, double> thickness_domain,
                                      FieldType type = FieldType::CUSTOM);

        /**
         * @brief Returns if the detector has a weighting potential in the sensor
         * @return True if the detector has a weighting potential, false otherwise
         */
        bool hasWeightingPotential() const;
        /**
         * @brief Return the type of weighting potential that is simulated.
         * @return The type of the weighting potential
         */
        FieldType getWeightingPotentialType() const;

        /**
         * @brief Get the weighting potential in the sensor at a local position
         * @param pos Position in the local frame
         * @param x x-coordinate of the pixel for which we want the weighting potential
         * @param y y-coordinate of the pixel for which we want the weighting potential
         * @return Value of the potential at the queried point
         */
        double getWeightingPotential(const ROOT::Math::XYZPoint& local_pos, const Pixel::Index& reference) const;

        /**
         * @brief Set the weighting potential in a single pixel in the detector using a grid
         * @param potential Flat array of the potential vectors (see detailed description)
         * @param sizes The dimensions of the flat weighting potential array
         * @param thickness_domain Domain in local coordinates in the thickness direction where the potential holds
         */
        void setWeightingPotentialGrid(const std::shared_ptr<std::vector<double>>& potential,
                                       std::array<size_t, 3> sizes,
                                       std::array<double, 2> scales,
                                       std::array<double, 2> offset,
                                       std::pair<double, double> thickness_domain);
        /**
         * @brief Set the weighting potential in a single pixel using a function
         * @param function Function used to retrieve the weighting potential
         * @param type Type of the weighting potential function used
         * @param thickness_domain Domain in local coordinates in the thickness direction where the potential holds
         */
        void setWeightingPotentialFunction(FieldFunction<double> function,
                                           std::pair<double, double> thickness_domain,
                                           FieldType type = FieldType::CUSTOM);

        /**
         * @brief Set the magnetic field in the detector
         * @param function Function used to retrieve the magnetic field
         * @param type Type of the magnetic field function used
         */
        void setMagneticField(ROOT::Math::XYZVector b_field);

        /**
         * @brief Returns if the detector has a magnetic field in the sensor
         * @return True if the detector has an magnetic field, false otherwise
         */
        bool hasMagneticField() const;
        /**
         * @brief Get the magnetic field in the sensor at a local position
         * @param pos Position in the local frame
         * @return Vector of the field at the queried point
         */
        ROOT::Math::XYZVector getMagneticField() const;

        /**
         * @brief Get the model of this detector
         * @return Pointer to the constant detector model
         */
        const std::shared_ptr<DetectorModel> getModel() const;

        /**
         * @brief Fetch an external object linked to this detector
         * @param name Name of the external object
         * @return External object or null pointer if it does not exists
         */
        template <typename T> std::shared_ptr<T> getExternalObject(const std::string& name);
        /**
         * @brief Sets an external object linked to this detector
         * @param name Name of the external object
         * @param model External object of arbitrary type
         */
        template <typename T> void setExternalObject(const std::string& name, std::shared_ptr<T> model);

    private:
        /**
         * @brief Constructs a detector in the geometry without a model (added later by the \ref GeometryManager)
         * @param name Unique name of the detector
         * @param position Position in the world frame
         * @param orientation Rotation matrix representing the orientation
         */
        Detector(std::string name, ROOT::Math::XYZPoint position, const ROOT::Math::Rotation3D& orientation);

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
        ROOT::Math::Rotation3D orientation_;

        // Transform matrix from global to local coordinates
        ROOT::Math::Transform3D transform_;

        // Electric field
        DetectorField<ROOT::Math::XYZVector, 3> electric_field_;

        // Weighting potential
        DetectorField<double, 1> weighting_potential_;

        // Magnetic field properties
        ROOT::Math::XYZVector magnetic_field_;
        bool magnetic_field_on_;

        std::map<std::type_index, std::map<std::string, std::shared_ptr<void>>> external_objects_;
    };

    /**
     * If the returned object is not a null pointer it is guaranteed to be of the correct type
     */
    template <typename T> std::shared_ptr<T> Detector::getExternalObject(const std::string& name) {
        return std::static_pointer_cast<T>(external_objects_[typeid(T)][name]);
    }
    /**
     * Stores external representations of objects in this detector that need to be shared between modules.
     */
    template <typename T> void Detector::setExternalObject(const std::string& name, std::shared_ptr<T> model) {
        external_objects_[typeid(T)][name] = std::static_pointer_cast<void>(model);
    }
} // namespace allpix

#endif /* ALLPIX_DETECTOR_H */
