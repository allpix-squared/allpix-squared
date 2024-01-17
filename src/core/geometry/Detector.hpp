/**
 * @file
 * @brief Base of detector implementation
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
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
        const std::string& getName() const { return name_; }

        /**
         * @brief Get type of the detector
         * @return Type of the detector model
         */
        const std::string& getType() const { return model_->getType(); }

        /**
         * @brief Get position in the world
         * @return Global position in Cartesian coordinates
         */
        const ROOT::Math::XYZPoint& getPosition() const { return position_; }
        /**
         * @brief Get orientation in the world
         * @return Rotation matrix representing the orientation
         */
        const ROOT::Math::Rotation3D& getOrientation() const { return orientation_; }

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
         * @brief Return a pixel object from the x- and y-index values
         * @return Pixel object
         */
        Pixel getPixel(int x, int y) const;

        /**
         * @brief Return a pixel object from the pixel index
         * @return Pixel object
         */
        Pixel getPixel(const Pixel::Index& index) const;

        /**
         * @brief Returns if the detector has an electric field in the sensor
         * @return True if the detector has an electric field, false otherwise
         */
        bool hasElectricField() const { return electric_field_.isValid(); }
        /**
         * @brief Return the type of electric field that is simulated.
         * @note The type of the electric field is set depending on the function used to apply it
         * @return The type of the electric field
         */
        FieldType getElectricFieldType() const { return electric_field_.getType(); }
        /**
         * @brief Get the electric field in the sensor at a local position
         * @param local_pos Position in the local frame
         * @return Vector of the field at the queried point
         */
        ROOT::Math::XYZVector getElectricField(const ROOT::Math::XYZPoint& local_pos) const;

        /**
         * @brief Set the electric field in a single pixel in the detector using a grid
         * @param field Flat array of the field vectors (see detailed description)
         * @param bins The dimensions of the flat electric field array
         * @param size Size of the electric field along the three dimensions of the field map
         * @param mapping Specification of the mapping of the field onto the pixel plane
         * @param scales Scaling factors for the field size, given in fractions of the field size in x and y
         * @param offset Offset of the field, given in fractions of the field size in x and y
         * @param thickness_domain Domain in local coordinates in the thickness direction where the field holds
         */
        void setElectricFieldGrid(const std::shared_ptr<std::vector<double>>& field,
                                  std::array<size_t, 3> bins,
                                  std::array<double, 3> size,
                                  FieldMapping mapping,
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
         * @brief Returns if the detector has a doping profile in the sensor
         * @return True if the detector has a doping profile, false otherwise
         */
        bool hasDopingProfile() const { return doping_profile_.isValid(); }
        /**
         * @brief Return the type of doping profile that is simulated.
         * @note The type of the doping profile is set depending on the function used to apply it
         * @return The type of the doping profile
         */
        FieldType getDopingProfileType() const { return doping_profile_.getType(); }
        /**
         * @brief Get the doping profile in the sensor at a local position
         * @param pos Position in the local frame
         * @return Value of the field at the queried point
         */
        double getDopingConcentration(const ROOT::Math::XYZPoint& local_pos) const;

        /**
         * @brief Set the doping profile in a single pixel in the detector using a grid
         * @param field Flat array of the field (see detailed description)
         * @param bins The dimensions of the flat doping profile array
         * @param size Size of the doping profile along the three dimensions of the field map
         * @param mapping Specification of the mapping of the field onto the pixel plane
         * @param scales Scaling factors for the field size, given in fractions of a pixel unit cell in x and y
         * @param offset Offset of the field, given in fractions of the field size in x and y
         * @param thickness_domain Domain in local coordinates in the thickness direction where the profile holds
         */
        void setDopingProfileGrid(std::shared_ptr<std::vector<double>> field,
                                  std::array<size_t, 3> bins,
                                  std::array<double, 3> size,
                                  FieldMapping mapping,
                                  std::array<double, 2> scales,
                                  std::array<double, 2> offset,
                                  std::pair<double, double> thickness_domain);
        /**
         * @brief Set the doping profile in a single pixel using a function
         * @param function Function used to retrieve the doping profile
         * @param type Type of the doping profile function used
         */
        void setDopingProfileFunction(FieldFunction<double> function, FieldType type = FieldType::CUSTOM);

        /**
         * @brief Returns if the detector has a weighting potential in the sensor
         * @return True if the detector has a weighting potential, false otherwise
         */
        bool hasWeightingPotential() const { return weighting_potential_.isValid(); }
        /**
         * @brief Return the type of weighting potential that is simulated.
         * @note The type of the weighting potential is set depending on the function used to apply it
         * @return The type of the weighting potential
         */
        FieldType getWeightingPotentialType() const { return weighting_potential_.getType(); }

        /**
         * @brief Get the weighting potential in the sensor at a local position
         * @param local_pos Position in the local frame
         * @param reference Index of the pixel for which we want the weighting potential
         * @return Value of the potential at the queried point
         */
        double getWeightingPotential(const ROOT::Math::XYZPoint& local_pos, const Pixel::Index& reference) const;

        /**
         * @brief Set the weighting potential in a single pixel in the detector using a grid
         * @param potential Flat array of the potential vectors (see detailed description)
         * @param bins The dimensions of the flat weighting potential array
         * @param size Size of the weighting potential along the three dimensions of the field map
         * @param mapping Specification of the mapping of the field onto the pixel plane
         * @param scales Scaling factors for the field size, given in fractions of a pixel unit cell in x and y
         * @param offset Offset of the field, given in fractions of the field size in x and y
         * @param thickness_domain Domain in local coordinates in the thickness direction where the potential holds
         */
        void setWeightingPotentialGrid(const std::shared_ptr<std::vector<double>>& potential,
                                       std::array<size_t, 3> bins,
                                       std::array<double, 3> size,
                                       FieldMapping mapping,
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
         * @param b_field Vector indicating strength and direction of the magnetic field
         */
        void setMagneticField(ROOT::Math::XYZVector b_field);

        /**
         * @brief Returns if the detector has a magnetic field in the sensor
         * @return True if the detector has an magnetic field, false otherwise
         */
        bool hasMagneticField() const { return magnetic_field_on_; }
        /**
         * @brief Get the magnetic field in the sensor at a local position
         * @param local_pos Position in the local frame
         * @return Vector of the field at the queried point
         */
        ROOT::Math::XYZVector getMagneticField(const ROOT::Math::XYZPoint& local_pos) const;

        /**
         * @brief Get the model of this detector
         * @return Pointer to the constant detector model
         */
        const std::shared_ptr<DetectorModel> getModel() const { return model_; }

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

        /**
         * @brief Check validity of the field map for this detector
         * @param size Size of the field in the three dimensions
         * @param mapping Specification of the mapping of the field onto the pixel plane
         * @param field_scale Scaling factors for the field
         * @param thickness_domain Thickness domain in which the field is defined in
         */
        void check_field_match(std::array<double, 3> size,
                               FieldMapping mapping,
                               std::array<double, 2> field_scale,
                               std::pair<double, double> thickness_domain) const;

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

        // Doping profile properties
        DetectorField<double, 1> doping_profile_;
    };

} // namespace allpix

#endif /* ALLPIX_DETECTOR_H */
