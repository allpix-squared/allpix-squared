
namespace allpix {
    /**
     * The electric field is replicated for all pixels and uses flipping at each boundary (side effects are not modeled in
     * this stage). Outside of the sensor the electric field is strictly zero by definition.
     */
    template <typename T, size_t N> T DetectorField<T, N>::get(const ROOT::Math::XYZPoint& pos) const {
        // FIXME: We need to revisit this to be faster and not too specific
        if(field_type_ == FieldType::NONE) {
            return {};
        }

        // Shift the coordinates by the offset configured for the electric field:
        auto x = pos.x() - field_offset_[0];
        auto y = pos.y() - field_offset_[1];
        auto z = pos.z();

        // Compute corresponding field replica coordinates:
        // WARNING This relies on the origin of the local coordinate system
        auto replica_x = static_cast<int>(std::floor((x + 0.5 * model_->getPixelSize().x()) * field_scales_inverse_[0]));
        auto replica_y = static_cast<int>(std::floor((y + 0.5 * model_->getPixelSize().y()) * field_scales_inverse_[1]));

        // Convert to the replica frame:
        x -= (replica_x + 0.5) * field_scales_[0] - 0.5 * model_->getPixelSize().x();
        y -= (replica_y + 0.5) * field_scales_[1] - 0.5 * model_->getPixelSize().y();

        // Do flipping if necessary
        if((replica_x % 2) == 1) {
            x *= -1;
        }
        if((replica_y % 2) == 1) {
            y *= -1;
        }

        // Compute using the grid or a function depending on the setting
        T ret_val;
        if(field_type_ == FieldType::GRID) {
            // Compute indices
            auto x_ind = static_cast<int>(
                std::floor(static_cast<double>(field_sizes_[0]) * (x + field_scales_[0] / 2.0) * field_scales_inverse_[0]));
            auto y_ind = static_cast<int>(
                std::floor(static_cast<double>(field_sizes_[1]) * (y + field_scales_[1] / 2.0) * field_scales_inverse_[1]));
            auto z_ind =
                static_cast<int>(std::floor(static_cast<double>(field_sizes_[2]) * (z - field_thickness_domain_.first) /
                                            (field_thickness_domain_.second - field_thickness_domain_.first)));

            // Check for indices within the sensor
            if(x_ind < 0 || x_ind >= static_cast<int>(field_sizes_[0]) || y_ind < 0 ||
               y_ind >= static_cast<int>(field_sizes_[1]) || z_ind < 0 || z_ind >= static_cast<int>(field_sizes_[2])) {
                return {};
            }

            // Compute total index
            size_t tot_ind = static_cast<size_t>(x_ind) * field_sizes_[1] * field_sizes_[2] * N +
                             static_cast<size_t>(y_ind) * field_sizes_[2] * N + static_cast<size_t>(z_ind) * N;

            // FIXME how to do in templated class?
            // ret_val = T((*field_)[tot_ind], (*field_)[tot_ind + 1], (*field_)[tot_ind + 2]);
            // auto test = ;
            ret_val = T(); //{get_impl(*field_, tot_ind, std::make_index_sequence<N>{})};
        } else {
            // Check if inside the thickness domain
            if(z < field_thickness_domain_.first || field_thickness_domain_.second < z) {
                return {};
            }

            // Calculate the electric field
            ret_val = field_function_(ROOT::Math::XYZPoint(x, y, z));
        }

        // FIXME - how to do this in templated class?
        // Flip vector if necessary
        // if((replica_x % 2) == 1) {
        //     ret_val.SetX(-ret_val.x());
        // }
        // if((replica_y % 2) == 1) {
        //     ret_val.SetY(-ret_val.y());
        // }

        return ret_val;
    }

    /**
     * The type of the electric field is set depending on the function used to apply it.
     */
    template <typename T, size_t N> FieldType DetectorField<T, N>::getType() const { return field_type_; }

    /**
     * @throws std::invalid_argument If the electric field sizes are incorrect or the thickness domain is outside the sensor
     *
     * The electric field is stored as a large flat array. If the sizes are denoted as respectively X_SIZE, Y_ SIZE and
     * Z_SIZE, each position (x, y, z) has three indices:
     * - x*Y_SIZE*Z_SIZE*3+y*Z_SIZE*3+z*3: the x-component of the electric field
     * - x*Y_SIZE*Z_SIZE*3+y*Z_SIZE*3+z*3+1: the y-component of the electric field
     * - x*Y_SIZE*Z_SIZE*3+y*Z_SIZE*3+z*3+2: the z-component of the electric field
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
        if(thickness_domain.first + 1e-9 < model_->getSensorCenter().z() - model_->getSensorSize().z() / 2.0 ||
           model_->getSensorCenter().z() + model_->getSensorSize().z() / 2.0 < thickness_domain.second - 1e-9) {
            throw std::invalid_argument("thickness domain is outside sensor dimensions");
        }
        if(thickness_domain.first >= thickness_domain.second) {
            throw std::invalid_argument("end of thickness domain is before begin");
        }

        field_ = std::move(field);
        field_sizes_ = sizes;

        // Precalculate the offset and scale of the field relative to the pixel pitch:
        field_scales_ =
            std::array<double, 2>{{model_->getPixelSize().x() * scales[0], model_->getPixelSize().y() * scales[1]}};
        field_scales_inverse_ = std::array<double, 2>{{1 / field_scales_[0], 1 / field_scales_[1]}};
        field_offset_ =
            std::array<double, 2>{{model_->getPixelSize().x() * offset[0], model_->getPixelSize().y() * offset[1]}};

        field_thickness_domain_ = std::move(thickness_domain);
        field_type_ = FieldType::GRID;
    }

    template <typename T, size_t N>
    void
    DetectorField<T, N>::setFunction(FieldFunction<T> function, std::pair<double, double> thickness_domain, FieldType type) {
        field_thickness_domain_ = std::move(thickness_domain);
        field_function_ = std::move(function);
        field_type_ = type;
    }
} // namespace allpix
