/**
 * @file
 * @brief Template implementation of detector fields
 *
 * @copyright Copyright (c) 2018-2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

namespace allpix {
    /**
     * Get a value from the field assigned to a specific pixel. This means, we cannot wrap around at the pixel edges and
     * start using the field of the adjacent pixel, but need to calculate the total distance from the lookup point in local
     * coordinates to the field origin in the given pixel.
     */
    template <typename T, size_t N>
    T DetectorField<T, N>::getRelativeTo(const ROOT::Math::XYZPoint& pos,
                                         const ROOT::Math::XYPoint& ref,
                                         const bool extrapolate_z) const {
        if(type_ == FieldType::NONE) {
            return {};
        }

        // Calculate the coordinates relative to the reference point:
        auto x = pos.x() - ref.x();
        auto y = pos.y() - ref.y();
        auto z = pos.z();

        T ret_val;
        if(type_ == FieldType::GRID) {
            ret_val = get_field_from_grid(ROOT::Math::XYZPoint(x, y, z), extrapolate_z);
        } else {
            // Check if we need to extrapolate along the z axis or if is inside thickness domain:
            if(extrapolate_z) {
                z = std::clamp(z, thickness_domain_.first, thickness_domain_.second);
            } else if(z < thickness_domain_.first || thickness_domain_.second < z) {
                return {};
            }

            // Calculate the field from the configured function:
            ret_val = function_(ROOT::Math::XYZPoint(x, y, z));
        }

        return ret_val;
    }

    // Maps the field indices onto the range of -d/2 < x < d/2, where d is the scale of the field in coordinate x.
    // This means, {x,y,z} = (0,0,0) is in the center of the field.
    template <typename T, size_t N>
    T DetectorField<T, N>::get_field_from_grid(const ROOT::Math::XYZPoint& dist, const bool extrapolate_z) const {

        auto pitch = model_->getPixelSize();
        // Fold onto available field scale - flip coordinates if necessary
        double x =
            (scale_ == FieldScale::HALF_X || scale_ == FieldScale::QUARTER ? 2 * (-std::abs(dist.x()) / pitch.x() + 0.5)
                                                                           : (dist.x() / pitch.x() + 0.5));
        double y =
            (scale_ == FieldScale::HALF_Y || scale_ == FieldScale::QUARTER ? 2 * (-std::abs(dist.y()) / pitch.y() + 0.5)
                                                                           : (dist.y() / pitch.y() + 0.5));

        // Compute indices
        // If the number of bins in x or y is 1, the field is assumed to be 2-dimensional and the respective index
        // is forced to zero. This circumvents that the field size in the respective dimension would otherwise be zero
        auto x_ind = (dimensions_[0] == 1 ? 0 : static_cast<int>(std::floor(x * static_cast<double>(dimensions_[0]))));
        auto y_ind = (dimensions_[1] == 1 ? 0 : static_cast<int>(std::floor(y * static_cast<double>(dimensions_[1]))));
        auto z_ind = static_cast<int>(std::floor(static_cast<double>(dimensions_[2]) * (dist.z() - thickness_domain_.first) /
                                                 (thickness_domain_.second - thickness_domain_.first)));

        // Check for indices within the field map
        if(x_ind < 0 || x_ind >= static_cast<int>(dimensions_[0]) || y_ind < 0 ||
           y_ind >= static_cast<int>(dimensions_[1])) {
            return {};
        }

        // Check if we need to extrapolate along the z axis:
        if(extrapolate_z) {
            z_ind = std::clamp(z_ind, 0, static_cast<int>(dimensions_[2]) - 1);
        } else if(z_ind < 0 || z_ind >= static_cast<int>(dimensions_[2])) {
            return {};
        }

        // Compute total index
        size_t tot_ind = static_cast<size_t>(x_ind) * dimensions_[1] * dimensions_[2] * N +
                         static_cast<size_t>(y_ind) * dimensions_[2] * N + static_cast<size_t>(z_ind) * N;

        // Retrieve field
        auto field_vector = get_impl(tot_ind, std::make_index_sequence<N>{});
        flip_vector_components(field_vector,
                               (dist.x() > 0) && (scale_ == FieldScale::HALF_X || scale_ == FieldScale::QUARTER),
                               (dist.y() > 0) && (scale_ == FieldScale::HALF_Y || scale_ == FieldScale::QUARTER));
        return field_vector;
    }

    /**
     * The field is replicated for all pixels and uses flipping at each boundary (edge effects are currently not modeled.
     * Outside of the sensor the field is strictly zero by definition.
     */
    template <typename T, size_t N>
    T DetectorField<T, N>::get(const ROOT::Math::XYZPoint& pos, const bool extrapolate_z) const {

        // FIXME: We need to revisit this to be faster and not too specific
        if(type_ == FieldType::NONE) {
            return {};
        }

        // Calculate which pixel we are in and the pixel center:
        auto [px, py] = model_->getPixelIndex(pos);
        auto pxpos = model_->getPixelCenter(static_cast<unsigned int>(px), static_cast<unsigned int>(py));

        // Calculate difference, add offset:
        auto diff = pos - pxpos + ROOT::Math::XYZVector(offset_[0], offset_[1], 0);

        // Compute using the grid or a function depending on the setting
        if(type_ == FieldType::GRID) {
            return get_field_from_grid(static_cast<ROOT::Math::XYZPoint>(diff), extrapolate_z);
        } else {
            // Check if we need to extrapolate along the z axis or if is inside thickness domain:
            if(extrapolate_z) {
                diff.SetZ(std::clamp(diff.z(), thickness_domain_.first, thickness_domain_.second));
            } else if(diff.z() < thickness_domain_.first || thickness_domain_.second < diff.z()) {
                return {};
            }

            // Calculate the field from the configured function:
            return function_(static_cast<ROOT::Math::XYZPoint>(diff));
        }
    }

    /**
     * Woohoo, template magic! Using an index_sequence to construct the templated return type with a variable number of
     * elements from the flat field vector, e.g. 3 for a vector field and 1 for a scalar field. Using a braced-init-list
     * allows to call the appropriate constructor of the return type, e.g. ROOT::Math::XYZVector or simply a double.
     */
    template <typename T, size_t N>
    template <std::size_t... I>
    auto DetectorField<T, N>::get_impl(size_t offset, std::index_sequence<I...>) const {
        return T{(*field_)[offset + I]...};
    }

    /**
     * The type of the field is set depending on the function used to apply it.
     */
    template <typename T, size_t N> FieldType DetectorField<T, N>::getType() const { return type_; }

    /**
     * @throws std::invalid_argument If the field dimensions are incorrect or the thickness domain is outside the sensor
     */
    template <typename T, size_t N>
    void DetectorField<T, N>::setGrid(std::shared_ptr<std::vector<double>> field, // NOLINT
                                      std::array<size_t, 3> dimensions,
                                      FieldScale scale,
                                      std::array<double, 2> offset,
                                      std::pair<double, double> thickness_domain) {
        if(model_ == nullptr) {
            throw std::invalid_argument("field not initialized with detector model parameters");
        }
        if(dimensions[0] * dimensions[1] * dimensions[2] * N != field->size()) {
            throw std::invalid_argument("field does not match the given dimensions");
        }
        if(thickness_domain.first + 1e-9 < model_->getSensorCenter().z() - model_->getSensorSize().z() / 2.0 ||
           model_->getSensorCenter().z() + model_->getSensorSize().z() / 2.0 < thickness_domain.second - 1e-9) {
            throw std::invalid_argument("thickness domain is outside sensor dimensions");
        }
        if(thickness_domain.first >= thickness_domain.second) {
            throw std::invalid_argument("end of thickness domain is before begin");
        }

        field_ = std::move(field);
        dimensions_ = dimensions;
        scale_ = scale;
        offset_ = offset;

        thickness_domain_ = std::move(thickness_domain);
        type_ = FieldType::GRID;
    }

    template <typename T, size_t N>
    void
    DetectorField<T, N>::setFunction(FieldFunction<T> function, std::pair<double, double> thickness_domain, FieldType type) {
        thickness_domain_ = std::move(thickness_domain);
        function_ = std::move(function);
        type_ = type;
    }
} // namespace allpix
