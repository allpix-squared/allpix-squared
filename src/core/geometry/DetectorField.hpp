/**
 * @file
 * @brief Definition of detector fields
 *
 * @copyright Copyright (c) 2019-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_DETECTOR_FIELD_H
#define ALLPIX_DETECTOR_FIELD_H

#include <array>
#include <functional>
#include <vector>

#include <Math/Point2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

#include "objects/Pixel.hpp"
#include "tools/ROOT.h"

namespace allpix {

    /**
     * @brief Type of fields
     */
    enum class FieldType {
        NONE = 0, ///< No field is applied
        CONSTANT, ///< Constant field
        LINEAR,   ///< Linear field (linearity determined by function)
        GRID,     ///< Field supplied through a regularized grid
        CUSTOM,   ///< Custom field function
    };

    /**
     * @brief Functor returning the field at a given position
     * @param pos Position in local coordinates at which the field should be evaluated
     */
    template <typename T = ROOT::Math::XYZVector> using FieldFunction = std::function<T(const ROOT::Math::XYZPoint& pos)>;

    /**
     * @brief Helper function to invert the field vector when flipping the field direction at pixel/field boundaries
     * @param field Field value, templated to support vector fields and scalar fields
     * @param x     Boolean to indicate flipping in x-direction
     * @param y     Boolean to indicate flipping in y-direction
     */
    template <typename T> void flip_vector_components(T& field, bool x, bool y);

    /**
     * @brief Field instance of a detector
     *
     * Contains the a pointer to the field dat along with the field sizes, binning and potential field distortions such as
     * scaling or offset parameters.
     */
    template <typename T, size_t N = 3> class DetectorField {
        friend class Detector;

    public:
        /**
         * @brief Constructs a detector field
         */
        DetectorField() = default;

        /**
         * @brief Check if the field is valid and either a field grid or a field function is configured
         * @return Boolean indicating field validity
         */
        bool isValid() const { return function_ || (dimensions_[0] != 0 && dimensions_[1] != 0 && dimensions_[2] != 0); };

        /**
         * @brief Return the type of field
         * @return The type of the field
         */
        FieldType getType() const;

        /**
         * @brief Get the field value in the sensor at a position provided in local coordinates
         * @param pos Position in the local frame
         * @param extrapolate_z Extrapolate the field along z when outside the defined region
         * @return Value(s) of the field at the queried point
         */
        T get(const ROOT::Math::XYZPoint& local_pos, const bool extrapolate_z = false) const;

        /**
         * @brief Get the value of the field at a position provided in local coordinates with respect to the reference
         * @param pos       Position in the local frame
         * @param reference Reference position to calculate the field for, x and y coordinate only
         * @param extrapolate_z Extrapolate the field along z when outside the defined region
         * @return Value(s) of the field assigned to the reference pixel at the queried point
         */
        T getRelativeTo(const ROOT::Math::XYZPoint& local_pos,
                        const ROOT::Math::XYPoint& reference,
                        const bool extrapolate_z = false) const;

        /**
         * @brief Set the field in the detector using a grid
         * @param field Flat array of the field
         * @param dimensions The dimensions of the flat field array
         * @param scales The actual physical extent of the field in each direction in x and y
         * @param offset Offset of the field in x and y, given in physical units
         * @param thickness_domain Domain in local coordinates in the thickness direction where the field holds
         */
        void setGrid(std::shared_ptr<std::vector<double>> field,
                     std::array<size_t, 3> dimensions,
                     std::array<double, 2> scales,
                     std::array<double, 2> offset,
                     std::pair<double, double> thickness_domain);
        /**
         * @brief Set the field in the detector using a function
         * @param function Function used to calculate the field
         * @param type Type of the field function used
         * @param thickness_domain Domain in local coordinates in the thickness direction where the field holds
         */
        void setFunction(FieldFunction<T> function,
                         std::pair<double, double> thickness_domain,
                         FieldType type = FieldType::CUSTOM);

    private:
        /**
         * @brief Set the relevant parameters from the detector model this field is used for
         * @param sensor_center The center of the sensor in local coordinates
         * @param sensor_size The extend of the sensor
         * @param pixel_pitch the pitch in X and Y of a single pixel
         */
        void set_model_parameters(const ROOT::Math::XYZPoint& sensor_center,
                                  const ROOT::Math::XYZVector& sensor_size,
                                  const ROOT::Math::XYVector& pixel_pitch) {
            sensor_center_ = sensor_center;
            sensor_size_ = sensor_size;
            pixel_size_ = pixel_pitch;
            model_initialized_ = true;
        }

        /**
         * @brief Helper function to retrieve the return type from a calculated index of the field data vector
         * @param offset The calculated global index to start from
         * @param index sequence expanded to the number of elements requested, depending on the template instance
         */
        template <std::size_t... I> auto get_impl(size_t offset, std::index_sequence<I...>) const;

        /**
         * @brief Helper function to calculate the field index based on the distance from its center and to return the values
         * @param dist Distance from the center of the field to obtain the values for, given in local coordinates
         * @param extrapolate_z Switch to either extrapolate the field along z when outside the grid or return zero
         * @return Value(s) of the field at the queried point
         */
        T get_field_from_grid(const ROOT::Math::XYZPoint& dist, const bool extrapolate_z = false) const;

        /**
         * Field properties
         * * Dimensions of the field map (bins in x, y, z)
         * * Scale of the field in x and y direction, defaults to 1, 1, i.e. to one full pixel cell, provided in fractions
         *   of the pixel pitch.
         * * Offset of the field from the pixel edge, e.g. when using fields centered at a pixel corner instead of the center
         *   Values provided as absolute shifts in um.
         */
        std::array<size_t, 3> dimensions_{};
        std::array<double_t, 2> scales_{{1., 1.}};
        std::array<double_t, 2> offset_{{0., 0.}};

        /**
         * Field definition
         * The field is either specified through a field grid, which is stored in a flat vector, or as field function
         * returning the value at each position given in local coordinates. The field is valid within the thickness domain
         * specified, the configured type is stored to allow additional checks in the modules requesting the field.
         *
         * In case of using a field grid, the field is stored as a large flat array. If the sizes are denoted as X_SIZE, Y_
         * SIZE and Z_SIZE, respectively, and each position (x, y, z) has N indices, the element position of the i-th field
         * component in the flat field vector can be calculated as:
         *
         *   field_i(x, y, z) =  x * Y_SIZE* Z_SIZE * N + y * Z_SIZE * + z * N + i
         */
        std::shared_ptr<std::vector<double>> field_;
        std::pair<double, double> thickness_domain_{};
        FieldType type_{FieldType::NONE};
        FieldFunction<T> function_;

        /*
         * Relevant parameters from the detector model for this field
         */
        ROOT::Math::XYVector pixel_size_{};
        ROOT::Math::XYZPoint sensor_center_{};
        ROOT::Math::XYZVector sensor_size_{};
        bool model_initialized_{};
    };
} // namespace allpix

// Include template members
#include "DetectorField.tpp"

#endif /* ALLPIX_DETECTOR_FIELD_H */
