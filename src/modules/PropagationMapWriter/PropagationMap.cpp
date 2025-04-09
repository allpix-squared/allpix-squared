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

using namespace allpix;

PropagationMap::PropagationMap(const std::shared_ptr<DetectorModel>& model,
                               std::array<size_t, 3> bins,
                               std::array<double, 3> size,
                               FieldMapping mapping,
                               std::array<double, 2> scales,
                               std::array<double, 2> offset,
                               std::pair<double, double> thickness_domain) {

    // Set detector model:
    setModel(model);

    field_ = std::make_shared<std::vector<double>>();
    field_->resize(bins[0] * bins[1] * bins[2]);

    // Calculate grid extent
    set_grid_parameters(bins, size, mapping, scales, offset, std::move(thickness_domain));
}

void PropagationMap::set(const ROOT::Math::XYZPoint& local_pos, const Pixel::Index final, double charge) {

    // FIXME check for mapping_ != FieldMapping::SENSOR)

    if(local_pos.z() < thickness_domain_.first || thickness_domain_.second < local_pos.z()) {
        return;
    }

    // Get initial pixel index from model and calculate relative position to final index location
    const auto [xpixel, ypixel] = model_->getPixelIndex(local_pos);
    const auto table_index = FieldTable::getIndex(final.x() - xpixel, final.x() - ypixel);

    // Calculate center of current pixel from index as reference point:
    auto ref = static_cast<ROOT::Math::XYPoint>(model_->getPixelCenter(xpixel, ypixel));

    // Map the coordinates onto the chosen pixel fraction
    auto [px, py, flip_x, flip_y] = map_coordinates(local_pos, ref);

    // Calculate the linearized index of the bin in the field vector
    const auto field_index = get_grid_index(px, py, local_pos.z(), false);

    // Set the field value from the given index
    (*field_)[field_index + table_index] += charge;
}
