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
         * @param model Model of the detector
         */
        DetectorField(std::shared_ptr<DetectorModel> model) : model_(model), field_type_(FieldType::NONE){};
        DetectorField() : model_(), field_type_(FieldType::NONE){};

        bool isValid() const { return initialized_; };

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
        bool initialized_{false};
        const std::shared_ptr<DetectorModel> model_;

        // Field properties
        std::array<size_t, 3> field_sizes_;
        std::array<double_t, 2> field_scales_{{1., 1.}};
        std::array<double_t, 2> field_scales_inverse_{{1., 1.}};
        std::array<double_t, 2> field_offset_{{0., 0.}};
        std::shared_ptr<std::vector<double>> field_;
        std::pair<double, double> field_thickness_domain_;
        FieldType field_type_{FieldType::NONE};
        FieldFunction<T> field_function_;
    };
} // namespace allpix

// Include template members
#include "DetectorField.tpp"

#endif /* ALLPIX_DETECTOR_FIELD_H */
