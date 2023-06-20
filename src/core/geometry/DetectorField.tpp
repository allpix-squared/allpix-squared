/**
 * @file
 * @brief Template implementation of detector fields
 *
 * @copyright Copyright (c) 2018-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

namespace allpix {

    /**
     * Get field relative to the center of the pixel the point is contained in.
     * Outside of the sensor the field is strictly zero by definition.
     */
    template <typename T, size_t N>
    T DetectorField<T, N>::get(const ROOT::Math::XYZPoint& pos, const bool extrapolate_z) const {

        // Return empty field if no field is set
        if(type_ == FieldType::NONE) {
            return {};
        }

        // Return empty field if outside the matrix
        if(!model_->isWithinMatrix(pos)) {
            return {};
        }

        // For per-pixel fields, resort to getRelativeTo with current pixel as reference:
        if(mapping_ != FieldMapping::SENSOR) {
            // Calculate center of current pixel from index as reference point:
            auto [px, py] = model_->getPixelIndex(pos);
            auto ref = static_cast<ROOT::Math::XYPoint>(model_->getPixelCenter(px, py));

            // Get field relative to pixel center:
            return getRelativeTo(pos, ref, extrapolate_z);
        }

        // Shift the coordinates by the offset configured for the field:
        auto x = pos.x() + offset_[0];
        auto y = pos.y() + offset_[1];
        auto z = pos.z();

        auto pitch = model_->getPixelSize();

        // Compute corresponding field replica coordinates:
        // WARNING This relies on the origin of the local coordinate system
        auto replica_x = static_cast<int>(std::floor((x + 0.5 * pitch.x()) * normalization_[0]));
        auto replica_y = static_cast<int>(std::floor((y + 0.5 * pitch.y()) * normalization_[1]));

        // Convert to the replica frame:
        x -= (replica_x + 0.5) / normalization_[0] - 0.5 * pitch.x();
        y -= (replica_y + 0.5) / normalization_[1] - 0.5 * pitch.y();

        // Do flipping if necessary
        x *= ((replica_x % 2) == 1 ? -1 : 1);
        y *= ((replica_y % 2) == 1 ? -1 : 1);

        // Compute using the grid or a function depending on the setting
        T ret_val;
        if(type_ == FieldType::GRID) {
            ret_val = get_field_from_grid(
                ROOT::Math::XYZPoint(x * normalization_[0] + 0.5, y * normalization_[1] + 0.5, pos.z()), extrapolate_z);
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

        // Flip vector if necessary
        flip_vector_components(ret_val, replica_x % 2, replica_y % 2);
        return ret_val;
    }

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
        auto x = pos.x() - ref.x() + offset_[0];
        auto y = pos.y() - ref.y() + offset_[1];
        auto z = pos.z();

        T ret_val;
        if(type_ == FieldType::GRID) {

            // Do we need to flip the position vector components?
            auto flip_x =
                (x > 0 && (mapping_ == FieldMapping::PIXEL_QUADRANT_II || mapping_ == FieldMapping::PIXEL_QUADRANT_III ||
                           mapping_ == FieldMapping::PIXEL_HALF_LEFT)) ||
                (x < 0 && (mapping_ == FieldMapping::PIXEL_QUADRANT_I || mapping_ == FieldMapping::PIXEL_QUADRANT_IV ||
                           mapping_ == FieldMapping::PIXEL_HALF_RIGHT));
            auto flip_y =
                (y > 0 && (mapping_ == FieldMapping::PIXEL_QUADRANT_III || mapping_ == FieldMapping::PIXEL_QUADRANT_IV ||
                           mapping_ == FieldMapping::PIXEL_HALF_BOTTOM)) ||
                (y < 0 && (mapping_ == FieldMapping::PIXEL_QUADRANT_I || mapping_ == FieldMapping::PIXEL_QUADRANT_II ||
                           mapping_ == FieldMapping::PIXEL_HALF_TOP));

            // Fold onto available field scale in the range [0 , 1] - flip coordinates if necessary
            auto px = (flip_x ? -1.0 : 1.0) * x * normalization_[0];
            auto py = (flip_y ? -1.0 : 1.0) * y * normalization_[1];

            if(mapping_ == FieldMapping::PIXEL_QUADRANT_II || mapping_ == FieldMapping::PIXEL_QUADRANT_III ||
               mapping_ == FieldMapping::PIXEL_HALF_LEFT) {
                px += 1.0;
            } else if(mapping_ == FieldMapping::PIXEL_FULL || mapping_ == FieldMapping::PIXEL_HALF_TOP ||
                      mapping_ == FieldMapping::PIXEL_HALF_BOTTOM) {
                px += 0.5;
            }

            if(mapping_ == FieldMapping::PIXEL_QUADRANT_III || mapping_ == FieldMapping::PIXEL_QUADRANT_IV ||
               mapping_ == FieldMapping::PIXEL_HALF_BOTTOM) {
                py += 1.0;
            } else if(mapping_ == FieldMapping::PIXEL_FULL || mapping_ == FieldMapping::PIXEL_HALF_LEFT ||
                      mapping_ == FieldMapping::PIXEL_HALF_RIGHT) {
                py += 0.5;
            }

            // Shuffle quadrants for inverted maps
            if(mapping_ == FieldMapping::PIXEL_FULL_INVERSE) {
                px += (x >= 0 ? 0. : 1.0);
                py += (y >= 0 ? 0. : 1.0);
            }

            ret_val = get_field_from_grid(ROOT::Math::XYZPoint(px, py, z), extrapolate_z, flip_x, flip_y);
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
    T DetectorField<T, N>::get_field_from_grid(const ROOT::Math::XYZPoint& dist,
                                               const bool extrapolate_z,
                                               const bool flip_x,
                                               const bool flip_y) const {

        // Compute indices
        // If the number of bins in x or y is 1, the field is assumed to be 2-dimensional and the respective index
        // is forced to zero. This circumvents that the field size in the respective dimension would otherwise be zero
        auto x_ind = (bins_[0] == 1 ? 0 : static_cast<int>(std::floor(dist.x() * static_cast<double>(bins_[0]))));
        if(x_ind < 0 || x_ind >= static_cast<int>(bins_[0])) {
            return {};
        }

        auto y_ind = (bins_[1] == 1 ? 0 : static_cast<int>(std::floor(dist.y() * static_cast<double>(bins_[1]))));
        if(y_ind < 0 || y_ind >= static_cast<int>(bins_[1])) {
            return {};
        }

        auto z_ind = static_cast<int>(std::floor(static_cast<double>(bins_[2]) * (dist.z() - thickness_domain_.first) /
                                                 (thickness_domain_.second - thickness_domain_.first)));

        // Check if we need to extrapolate along the z axis:
        if(extrapolate_z) {
            z_ind = std::clamp(z_ind, 0, static_cast<int>(bins_[2]) - 1);
        } else if(z_ind < 0 || z_ind >= static_cast<int>(bins_[2])) {
            return {};
        }

        // Compute total index
        size_t tot_ind = static_cast<size_t>(x_ind) * bins_[1] * bins_[2] * N + static_cast<size_t>(y_ind) * bins_[2] * N +
                         static_cast<size_t>(z_ind) * N;

        // Retrieve field
        auto field_vector = get_impl(tot_ind, std::make_index_sequence<N>{});
        // Flip sign of vector components if necessary
        flip_vector_components(field_vector, flip_x, flip_y);
        return field_vector;
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
     * @throws std::invalid_argument If the field bins are incorrect or the thickness domain is outside the sensor
     */
    template <typename T, size_t N>
    void DetectorField<T, N>::setGrid(std::shared_ptr<std::vector<double>> field, // NOLINT
                                      std::array<size_t, 3> bins,
                                      std::array<double, 3> size,
                                      FieldMapping mapping,
                                      std::array<double, 2> scales,
                                      std::array<double, 2> offset,
                                      std::pair<double, double> thickness_domain) {
        if(model_ == nullptr) {
            throw std::invalid_argument("field not initialized with detector model parameters");
        }
        if(bins[0] * bins[1] * bins[2] * N != field->size()) {
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
        bins_ = bins;
        mapping_ = mapping;

        // Calculate normalization of field from field size and scale factors:
        normalization_[0] = 1.0 / scales[0] / size[0];
        normalization_[1] = 1.0 / scales[1] / size[1];
        offset_[0] = offset[0] * size[0];
        offset_[1] = offset[1] * size[1];

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
