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
     * Get field relative to the center of the pixel the point is contained in.
     * Outside of the sensor the field is strictly zero by definition.
     */
    template <typename T, size_t N>
    T DetectorField<T, N>::get(const ROOT::Math::XYZPoint& pos, const bool extrapolate_z) const {

        // Calculate current pixel index its center as reference point:
        auto [px, py] = model_->getPixelIndex(pos);
        if(!model_->isWithinMatrix(px, py)) {
            return {};
        }
        auto ref = static_cast<ROOT::Math::XYPoint>(
            model_->getPixelCenter(static_cast<unsigned int>(px), static_cast<unsigned int>(py)));

        // Get field relative to pixel center:
        return getRelativeTo(pos, ref, extrapolate_z);
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

        // Do we need to flip the position vector components?
        auto flip_x = (dist.x() > 0 && (mapping_ == FieldMapping::QUADRANT_II || mapping_ == FieldMapping::QUADRANT_III ||
                                        mapping_ == FieldMapping::HALF_LEFT)) ||
                      (dist.x() < 0 && (mapping_ == FieldMapping::QUADRANT_I || mapping_ == FieldMapping::QUADRANT_IV ||
                                        mapping_ == FieldMapping::HALF_RIGHT));
        auto flip_y = (dist.y() > 0 && (mapping_ == FieldMapping::QUADRANT_III || mapping_ == FieldMapping::QUADRANT_IV ||
                                        mapping_ == FieldMapping::HALF_BOTTOM)) ||
                      (dist.y() < 0 && (mapping_ == FieldMapping::QUADRANT_I || mapping_ == FieldMapping::QUADRANT_II ||
                                        mapping_ == FieldMapping::HALF_TOP));

        // Fold onto available field scale in the range [0 , 1] - flip coordinates if necessary
        auto x = (flip_x ? -1.0 : 1.0) * dist.x() * normalization_[0];
        auto y = (flip_y ? -1.0 : 1.0) * dist.y() * normalization_[1];

        if(mapping_ == FieldMapping::QUADRANT_II || mapping_ == FieldMapping::QUADRANT_III ||
           mapping_ == FieldMapping::HALF_LEFT) {
            x += 1.0;
        } else if(mapping_ == FieldMapping::FULL) {
            x += 0.5;
        }

        if(mapping_ == FieldMapping::QUADRANT_III || mapping_ == FieldMapping::QUADRANT_IV ||
           mapping_ == FieldMapping::HALF_BOTTOM) {
            y += 1.0;
        } else if(mapping_ == FieldMapping::FULL) {
            y += 0.5;
        }

        // Shuffle quadrants for inverted maps
        if(mapping_ == FieldMapping::FULL_INVERSE) {
            x += (dist.x() > 0 ? 0. : 1.0);
            y += (dist.y() > 0 ? 0. : 1.0);
        }

        // Compute indices
        // If the number of bins in x or y is 1, the field is assumed to be 2-dimensional and the respective index
        // is forced to zero. This circumvents that the field size in the respective dimension would otherwise be zero
        auto x_ind = (bins_[0] == 1 ? 0 : static_cast<int>(std::floor(x * static_cast<double>(bins_[0]))));
        if(x_ind < 0 || x_ind >= static_cast<int>(bins_[0])) {
            return {};
        }

        auto y_ind = (bins_[1] == 1 ? 0 : static_cast<int>(std::floor(y * static_cast<double>(bins_[1]))));
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
     * The type of the field is set depending on the function used to apply it.
     */
    template <typename T, size_t N> FieldType DetectorField<T, N>::getType() const { return type_; }

    /**
     * @throws std::invalid_argument If the field bins are incorrect or the thickness domain is outside the sensor
     */
    template <typename T, size_t N>
    void DetectorField<T, N>::setGrid(std::shared_ptr<std::vector<double>> field, // NOLINT
                                      std::array<size_t, 3> bins,
                                      FieldMapping mapping,
                                      std::array<double, 2> scales,
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

        // Calculate normalization of field sizes from pitch and scales:
        auto pitch = model_->getPixelSize();
        normalization_[0] = 1.0 / scales[0] / pitch.x();
        normalization_[1] = 1.0 / scales[1] / pitch.y();

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
