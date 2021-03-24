/**
 * @file
 * @brief Template implementation of detector fields
 *
 * @copyright Copyright (c) 2019-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
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
                // TODO When moving to C++17, this can be replaced with std::clamp()
                z = std::max(thickness_domain_.first, std::min(z, thickness_domain_.second));
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
        // Compute indices
        // If the number of bins in x or y is 1, the field is assumed to be 2-dimensional and the respective index
        // is forced to zero. This circumvents that the field size in the respective dimension would otherwise be zero
        // clang-format off
        auto x_ind = (dimensions_[0] == 1 ? 0
                                          : static_cast<int>(std::floor(static_cast<double>(dimensions_[0]) *
                                                                        (dist.x() + scales_[0] / 2.0) / scales_[0])));
        auto y_ind = (dimensions_[1] == 1 ? 0
                                          : static_cast<int>(std::floor(static_cast<double>(dimensions_[1]) *
                                                                        (dist.y() + scales_[1] / 2.0) / scales_[1])));
        auto z_ind = static_cast<int>(std::floor(static_cast<double>(dimensions_[2]) * (dist.z() - thickness_domain_.first) /
                                                 (thickness_domain_.second - thickness_domain_.first)));
        // clang-format on

        // Check for indices within the field map
        if(x_ind < 0 || x_ind >= static_cast<int>(dimensions_[0]) || y_ind < 0 ||
           y_ind >= static_cast<int>(dimensions_[1])) {
            return {};
        }

        // Check if we need to extrapolate along the z axis:
        if(extrapolate_z) {
            // TODO When moving to C++17, this can be replaced with std::clamp()
            z_ind = std::max(0, std::min(z_ind, static_cast<int>(dimensions_[2]) - 1));
        } else if(z_ind < 0 || z_ind >= static_cast<int>(dimensions_[2])) {
            return {};
        }

        // Compute total index
        size_t tot_ind = static_cast<size_t>(x_ind) * dimensions_[1] * dimensions_[2] * N +
                         static_cast<size_t>(y_ind) * dimensions_[2] * N + static_cast<size_t>(z_ind) * N;

        return get_impl(tot_ind, std::make_index_sequence<N>{});
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

        // Shift the coordinates by the offset configured for the field:
        auto x = pos.x() + offset_[0];
        auto y = pos.y() + offset_[1];
        auto z = pos.z();

        // Compute corresponding field replica coordinates:
        // WARNING This relies on the origin of the local coordinate system
        auto replica_x = static_cast<int>(std::floor((x + 0.5 * pixel_size_.x()) / scales_[0]));
        auto replica_y = static_cast<int>(std::floor((y + 0.5 * pixel_size_.y()) / scales_[1]));

        // Convert to the replica frame:
        x -= (replica_x + 0.5) * scales_[0] - 0.5 * pixel_size_.x();
        y -= (replica_y + 0.5) * scales_[1] - 0.5 * pixel_size_.y();

        // Do flipping if necessary
        if((replica_x % 2) == 1) {
            x *= -1;
        }
        if((replica_y % 2) == 1) {
            y *= -1;
        }

        // Compute using the grid or a function depending on the setting
        T ret_val;
        if(type_ == FieldType::GRID) {
            ret_val = get_field_from_grid(ROOT::Math::XYZPoint(x, y, z));
        } else {
            // Check if we need to extrapolate along the z axis or if is inside thickness domain:
            if(extrapolate_z) {
                // TODO When moving to C++17, this can be replaced with std::clamp()
                z = std::max(thickness_domain_.first, std::min(z, thickness_domain_.second));
            } else if(z < thickness_domain_.first || thickness_domain_.second < z) {
                return {};
            }

            // Calculate the field from the configured function:
            ret_val = function_(ROOT::Math::XYZPoint(x, y, z));
        }

        // Flip vector if necessary
        flip_vector_components(ret_val, replica_x % 2, replica_y % 2);
        return ret_val;
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
                                      std::array<double, 2> scales,
                                      std::array<double, 2> offset,
                                      std::pair<double, double> thickness_domain) {
        if(!model_initialized_) {
            throw std::invalid_argument("field not initialized with detector model parameters");
        }
        if(dimensions[0] * dimensions[1] * dimensions[2] * N != field->size()) {
            throw std::invalid_argument("field does not match the given dimensions");
        }
        if(thickness_domain.first + 1e-9 < sensor_center_.z() - sensor_size_.z() / 2.0 ||
           sensor_center_.z() + sensor_size_.z() / 2.0 < thickness_domain.second - 1e-9) {
            throw std::invalid_argument("thickness domain is outside sensor dimensions");
        }
        if(thickness_domain.first >= thickness_domain.second) {
            throw std::invalid_argument("end of thickness domain is before begin");
        }

        field_ = std::move(field);
        dimensions_ = dimensions;
        scales_ = scales;
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
