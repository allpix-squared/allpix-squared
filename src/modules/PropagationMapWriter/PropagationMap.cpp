/**
 * @file
 * @brief Implementation of propagation map
 *
 * @copyright Copyright (c) 2018-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "PropagationMap.hpp"

#include <mutex>

using namespace allpix;

PropagationMap::PropagationMap(const std::shared_ptr<DetectorModel>& model,
                               std::array<size_t, 3> bins,
                               std::array<double, 3> size,
                               FieldMapping mapping,
                               std::array<double, 2> scales,
                               std::array<double, 2> offset,
                               std::pair<double, double> thickness_domain) {

    const std::lock_guard field_lock {field_mutex_};

    // Set detector model:
    setModel(model);

    // Generate field and resize
    field_ = std::make_shared<std::vector<double>>();
    field_->resize(bins[0] * bins[1] * bins[2] * 25);

    // Keep track of how many tables we summed per bin
    normalization_table.resize(bins[0] * bins[1] * bins[2]);

    // Calculate grid extent
    set_grid_parameters(bins, size, mapping, scales, offset, std::move(thickness_domain));
}

void PropagationMap::add(const ROOT::Math::XYZPoint& local_pos, const FieldTable& table) {

    // FIXME check for mapping_ != FieldMapping::SENSOR)

    if(local_pos.z() < thickness_domain_.first || thickness_domain_.second < local_pos.z()) {
        return;
    }

    // Get initial pixel index from model and calculate relative position to final index location
    const auto [xpixel, ypixel] = model_->getPixelIndex(local_pos);

    // Calculate center of current pixel from index as reference point and map to chosen pixel fraction:
    auto ref = static_cast<ROOT::Math::XYPoint>(model_->getPixelCenter(xpixel, ypixel));
    auto [px, py, flip_x, flip_y] = map_coordinates(local_pos, ref);

    // Calculate the linearized index of the starting bin in the field vector
    const auto field_index = get_grid_index(px, py, local_pos.z(), false);

    const std::lock_guard field_lock {field_mutex_};
    // Add the map values starting from the given index
    for(size_t i = 0; i < 25; i++) {
        (*field_)[field_index + i] += table[i];
    }

    // Count up the number of tables summed in this bin:
    normalization_table[field_index / 25]++;
}

std::shared_ptr<std::vector<double>> PropagationMap::getNormalizedField() {
    const std::lock_guard field_lock {field_mutex_};

    // Loop over field and apply normalization:
    for(size_t i = 0; i < field_->size(); i++) {
        if(normalization_table[i / 25] > 0) {
            (*field_)[i] /= static_cast<double>(normalization_table[i / 25]);
        }
    }
    // Return field
    return field_;
}
