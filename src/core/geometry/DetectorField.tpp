
namespace allpix {
    /**
     * The field is replicated for all pixels and uses flipping at each boundary (edge effects are currently not modeled.
     * Outside of the sensor the field is strictly zero by definition.
     */
    template <typename T, size_t N> T DetectorField<T, N>::get(const ROOT::Math::XYZPoint& pos) const {
        // FIXME: We need to revisit this to be faster and not too specific
        if(type_ == FieldType::NONE) {
            return {};
        }

        // Shift the coordinates by the offset configured for the field:
        auto x = pos.x() - offset_[0];
        auto y = pos.y() - offset_[1];
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
            // Compute indices
            auto x_ind = static_cast<int>(std::floor(static_cast<double>(sizes_[0]) * (x + scales_[0] / 2.0) / scales_[0]));
            auto y_ind = static_cast<int>(std::floor(static_cast<double>(sizes_[1]) * (y + scales_[1] / 2.0) / scales_[1]));
            auto z_ind = static_cast<int>(std::floor(static_cast<double>(sizes_[2]) * (z - thickness_domain_.first) /
                                                     (thickness_domain_.second - thickness_domain_.first)));

            // Check for indices within the sensor
            if(x_ind < 0 || x_ind >= static_cast<int>(sizes_[0]) || y_ind < 0 || y_ind >= static_cast<int>(sizes_[1]) ||
               z_ind < 0 || z_ind >= static_cast<int>(sizes_[2])) {
                return {};
            }

            // Compute total index
            size_t tot_ind = static_cast<size_t>(x_ind) * sizes_[1] * sizes_[2] * N +
                             static_cast<size_t>(y_ind) * sizes_[2] * N + static_cast<size_t>(z_ind) * N;

            ret_val = get_impl(tot_ind, std::make_index_sequence<N>{});
        } else {
            // Check if inside the thickness domain
            if(z < thickness_domain_.first || thickness_domain_.second < z) {
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
     * @throws std::invalid_argument If the field sizes are incorrect or the thickness domain is outside the sensor
     */
    template <typename T, size_t N>
    void DetectorField<T, N>::setGrid(std::shared_ptr<std::vector<double>> field,
                                      std::array<size_t, 3> sizes,
                                      std::array<double, 2> scales,
                                      std::array<double, 2> offset,
                                      std::pair<double, double> thickness_domain) {
        if(sizes[0] * sizes[1] * sizes[2] * N != field->size()) {
            throw std::invalid_argument("field does not match the given sizes");
        }
        if(thickness_domain.first + 1e-9 < sensor_center_.z() - sensor_size_.z() / 2.0 ||
           sensor_center_.z() + sensor_size_.z() / 2.0 < thickness_domain.second - 1e-9) {
            throw std::invalid_argument("thickness domain is outside sensor dimensions");
        }
        if(thickness_domain.first >= thickness_domain.second) {
            throw std::invalid_argument("end of thickness domain is before begin");
        }

        field_ = std::move(field);
        sizes_ = sizes;

        // Precalculate the offset and scale of the field relative to the pixel pitch:
        scales_ = std::array<double, 2>{{pixel_size_.x() * scales[0], pixel_size_.y() * scales[1]}};
        offset_ = std::array<double, 2>{{pixel_size_.x() * offset[0], pixel_size_.y() * offset[1]}};

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
