/**
 * @file
 * @brief Definition of detector fields
 *
 * @copyright Copyright (c) 2018-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
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

#include "DetectorModel.hpp"
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
        CUSTOM1D, ///< Custom field function, dependent only on z
        CUSTOM,   ///< Custom field function
    };

    /**
     * @brief Type of field maps
     */
    enum class FieldMapping {
        PIXEL_FULL = 0,     ///< The field map spans the full pixel plane
        PIXEL_FULL_INVERSE, ///< The field map spans the full pixel plane, but pixel centers are at field corners
        PIXEL_HALF_LEFT,    ///< The field map spans the left half of the volume and is mirrored along x
        PIXEL_HALF_RIGHT,   ///< The field map spans the right half of the volume and is mirrored along x
        PIXEL_HALF_TOP,     ///< The field map spans the top half of the volume and is mirrored along y
        PIXEL_HALF_BOTTOM,  ///< The field map spans the bottom half of the volume and is mirrored along y
        PIXEL_QUADRANT_I,   ///< The field map spans the top right quadrant of the volume and is mirrored to the other
                            ///< quadrants
        PIXEL_QUADRANT_II,  ///< The field map spans the top left quadrant of the volume and is mirrored to the other
                            ///< quadrants
        PIXEL_QUADRANT_III, ///< The field map spans the lower left quadrant of the volume and is mirrored to the other
                            ///< quadrants
        PIXEL_QUADRANT_IV,  ///< The field map spans the lower right quadrant of the volume and is mirrored to the other
                            ///< quadrants
        SENSOR, ///< The field is mapped to the full sensor, starting at the local coordinate origin. The field is
                ///< mirrored at its edges.
    };

    /**
     * @brief Functor returning the field at a given position
     * @param pos Position in local coordinates at which the field should be evaluated
     */
    template <typename T = ROOT::Math::XYZVector> using FieldFunction = std::function<T(const ROOT::Math::XYZPoint& pos)>;

    /**
     * @brief FieldTable is a linearized 5x5 matrix
     */
    class FieldTable : public std::array<double, 25> {
    public:
        /**
         * @brief Helper function to translate an iterator of the array into a coordinate of the 5x5 matrix.
         *
         * The central pixel has coordinates 0,0, the others around positive or negative values, respectively. This allows
         * to directly add these coordinates to any pixel index of the sensor.
         *
         * @param it Iterator to the array
         * @return Pair of x and y coordinates.
         */
        std::pair<int, int> getCoordinates(const FieldTable::iterator& it) {
            const auto i = std::distance(this->begin(), it);
            return {static_cast<int>(i % 5) - 2, static_cast<int>(i / 5) - 2};
        }

        /**
         * @brief Helper to get the index in the field table from relative coordinates
         *
         * @param x Relative coordinate in x
         * @param y Relative coordinate in y
         *
         * @return field index
         */
        static size_t getIndex(int x, int y) { return static_cast<size_t>(y + 2) * 5 + static_cast<size_t>(x + 2); }
    };

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
     * Contains the a pointer to the field data along with the field sizes, binning and potential field distortions such as
     * scaling or offset parameters.
     */
    template <typename T, size_t N = 3> class DetectorField {

    public:
        /**
         * @brief Constructs a detector field
         */
        DetectorField() = default;

        /**
         * @brief Check if the field is valid and either a field grid or a field function is configured
         * @return Boolean indicating field validity
         */
        bool isValid() const { return function_ || (bins_[0] != 0 && bins_[1] != 0 && bins_[2] != 0); }

        /**
         * @brief Return the type of field
         * @note The type of the field is set depending on the function used to apply it
         * @return The type of the field
         */
        FieldType getType() const { return type_; }

        /**
         * @brief Get the field value in the sensor at a position provided in local coordinates
         * @param local_pos Position in the local frame
         * @param extrapolate_z Extrapolate the field along z when outside the defined region
         * @return Value(s) of the field at the queried point
         */
        T get(const ROOT::Math::XYZPoint& local_pos, const bool extrapolate_z = false) const;

        /**
         * @brief Get the value of the field at a position provided in local coordinates with respect to the reference
         * @param local_pos Position in the local frame
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
         * @param bins The bins of the flat field array
         * @param size Physical extent of the field
         * @param mapping Specification of the mapping of the field onto the pixel plane
         * @param scales Scaling factors for the field size, given in fractions of the field size in x and y
         * @param offset Offset of the field from the pixel center, given in fractions of the field size in x and y
         * @param thickness_domain Domain in local coordinates in the thickness direction where the field holds
         */
        void setGrid(std::shared_ptr<std::vector<double>> field,
                     std::array<size_t, 3> bins,
                     std::array<double, 3> size,
                     FieldMapping mapping,
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

        /**
         * @brief Set the detector model this field is used for
         * @param model The detector model
         */
        void setModel(const std::shared_ptr<DetectorModel>& model) { model_ = model; }

    protected:
        /**
         * @brief Helper to calculate field size normalization factors and configure them
         * @param bins The bins of the flat field array
         * @param size Physical extent of the field
         * @param mapping Specification of the mapping of the field onto the pixel plane
         * @param scales Scaling factors for the field size, given in fractions of the field size in x and y
         * @param offset Offset of the field from the pixel center, given in fractions of the field size in x and y
         * @param thickness_domain Domain in local coordinates in the thickness direction where the field holds
         */
        void set_grid_parameters(std::array<size_t, 3> bins,
                                 std::array<double, 3> size,
                                 FieldMapping mapping,
                                 std::array<double, 2> scales,
                                 std::array<double, 2> offset,
                                 std::pair<double, double> thickness_domain);

        /**
         * @brief Helper function to retrieve the return type from a calculated index of the field data vector
         * @param offset The calculated global index to start from
         * @note The index sequence is expanded to the number of elements requested, depending on the template instance
         */
        template <std::size_t... I> inline auto get_impl(size_t offset, std::index_sequence<I...>) const noexcept;

        /**
         * @brief Helper function to calculate the field index based on the distance from its center
         * @param index Absolute index in the field grid
         * @param x Distance in local-coordinate x from the center of the field to obtain the values for
         * @param y Distance in local-coordinate y from the center of the field to obtain the values for
         * @param z Distance in local-coordinate z from the center of the field to obtain the values for
         * @param extrapolate_z Flag whether we should extrapolate
         */
        inline bool get_grid_index(
            size_t& index, const double x, const double y, const double z, const bool extrapolate_z) const noexcept;

        /**
         * @brief Map x and y coordinates of a position and a reference point onto a pixel given the chosen mapping.
         *
         * @param pos Position to calculate coordinates for
         * @param ref Reference position to calculate relative position to
         *
         * @return Tuple with relative x and y coordinates, mapped into the chosen area, and booleans indicating whether
         *         flipping of vector components is necessary
         */
        inline std::tuple<double, double, bool, bool> map_coordinates(const ROOT::Math::XYZPoint& pos,
                                                                      const ROOT::Math::XYPoint& ref) const;

        /**
         * @brief Fast floor-to-int implementation without overflow protection as std::floor
         * @param x Double-precision floating point value
         * @return Integer floored towards negative infinity
         * */
        static inline int int_floor(double x) noexcept {
            auto i = static_cast<int>(x);
            return i - static_cast<int>(static_cast<double>(i) > x);
        };

        /**
         * Field properties
         * * bins of the field map (bins in x, y, z)
         * * Mapping of the field onto the pixel cell
         * * Scale of the field in x and y direction, defaults to one full pixel cell
         */
        std::array<size_t, 3> bins_{};
        FieldMapping mapping_{FieldMapping::PIXEL_FULL};
        std::array<double, 2> normalization_{{1., 1.}};
        std::array<double, 2> offset_{{0., 0.}};

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
        std::shared_ptr<DetectorModel> model_;
    };
} // namespace allpix

// Include template members
#include "DetectorField.tpp"

#endif /* ALLPIX_DETECTOR_FIELD_H */
