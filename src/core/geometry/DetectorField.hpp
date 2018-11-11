/**
 * @file
 * @brief Definition of detector fields
 *
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_DETECTOR_FIELD_H
#define ALLPIX_DETECTOR_FIELD_H

#include <array>
#include <functional>
#include <map>
#include <tuple>
#include <typeindex>
#include <vector>

#include "core/geometry/DetectorModel.hpp"

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

    template <typename T = ROOT::Math::XYZVector> using FieldFunction = std::function<T(const ROOT::Math::XYZPoint&)>;
    template <typename T> void flip_vector_components(T&, bool x, bool y);

    /**
     * @brief Field instance of a detector
     *
     * Contains the a pointer to the field dat along with the field sizes, binning and potential field distortions such as
     * scaling or offset parameters.
     */
    template <typename T, size_t N = 3> class DetectorField {

    public:
        /**
         * @brief Constructs a detector field
         */
        DetectorField() : field_type_(FieldType::NONE){};

        void setModelParameters(ROOT::Math::XYVector pixel_pitch, ROOT::Math::XYVector thickness) {
            pixel_size_ = pixel_pitch;
            sensor_thickness_ = thickness;
        }

        bool isValid() const {
            return field_function_ || (field_sizes_[0] != 0 && field_sizes_[1] != 0 && field_sizes_[2] != 0);
        };

        /**
         * @brief Return the type of field
         * @return The type of the field
         */
        FieldType getType() const;
        /**
         * @brief Get the field value in the sensor at a local position
         * @param pos Position in the local frame
         * @return Value(s) of the field at the queried point
         */
        T get(const ROOT::Math::XYZPoint& local_pos) const;

        /**
         * @brief Set the field in the detector using a grid
         * @param field Flat array of the field (see detailed description)
         * @param sizes The dimensions of the flat field array
         * @param scales Scaling factors for the field size, given in fractions of a pixel unit cell in x and y
         * @param thickness_domain Domain in local coordinates in the thickness direction where the field holds
         */
        void setGrid(std::shared_ptr<std::vector<double>> field,
                     std::array<size_t, 3> sizes,
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
        /*
         * @brief Helper function to retrieve the return type from a calculated index
         * @param a the field data vector
         * @param offset the calcilated global index to start from
         * @param index sequence expanded to the number of elements requested, depending on the template instance
         */
        template <std::size_t... I> auto get_impl(size_t offset, std::index_sequence<I...>) const;

        // Field properties
        std::array<size_t, 3> field_sizes_;

        /*
         * Scale of the field in x and y direction, defaults to 1, 1, i.e. to one full pixel cell
         * The inverse of the field scales is pre-calculated for convenience
         */
        std::array<double_t, 2> field_scales_{{1., 1.}};
        std::array<double_t, 2> field_scales_inverse_{{1., 1.}};

        /*
         * Offset of the field from the pixel edge, e.g. when using fields centered at a pixel corner instead of the center
         */
        std::array<double_t, 2> field_offset_{{0., 0.}};

        std::shared_ptr<std::vector<double>> field_;
        std::pair<double, double> field_thickness_domain_;
        FieldType field_type_{FieldType::NONE};
        FieldFunction<T> field_function_;

        ROOT::Math::XYVector pixel_size_;
        ROOT::Math::XYVector sensor_thickness_;
    };
} // namespace allpix

// Include template members
#include "DetectorField.tpp"

#endif /* ALLPIX_DETECTOR_FIELD_H */
