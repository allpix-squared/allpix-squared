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

#include <array>
#include <cstddef>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <Math/Point3D.h>

#include "core/geometry/DetectorField.hpp"
#include "core/geometry/DetectorModel.hpp"
#include "core/utils/log.h"

using namespace allpix;

PropagationMap::PropagationMap(const std::shared_ptr<DetectorModel>& model,
                               std::array<size_t, 3> bins,
                               std::array<double, 3> size,
                               FieldMapping mapping,
                               std::array<double, 2> scales,
                               std::array<double, 2> offset,
                               std::pair<double, double> thickness_domain) {

    const std::lock_guard field_lock{field_mutex_};

    // Set detector model:
    setModel(model);

    // Generate field and resize
    field_ = std::make_shared<std::vector<double>>();
    field_->resize(bins[0] * bins[1] * bins[2] * 25);

    // Keep track of how many tables we summed per bin
    normalization_table_.resize(bins[0] * bins[1] * bins[2]);

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
    size_t field_index = 0;
    if(!get_grid_index(field_index, px, py, local_pos.z(), false)) {
        // Outside the field, ignoring
        return;
    }

    const std::lock_guard field_lock{field_mutex_};
    // Add the map values starting from the given index
    for(size_t i = 0; i < 25; i++) {
        (*field_)[field_index + i] += table[i];
    }

    // Count up the number of tables summed in this bin:
    normalization_table_[field_index / 25]++;
}

void PropagationMap::checkField() {
    size_t empty_bins = 0;
    size_t low_statistics = 0;
    size_t statistics = 0;

    const std::lock_guard field_lock{field_mutex_};

    for(const auto& bin : normalization_table_) {
        empty_bins += (bin == 0 ? 1 : 0);
        low_statistics += (bin < 10 ? 1 : 0);
        statistics += bin;
    }
    if(low_statistics > 0) {
        LOG(WARNING) << low_statistics << " bins in output probability table have low statistics - result may be inaccurate";
        if(empty_bins > 0) {
            LOG(ERROR) << "Found " << empty_bins
                       << " bins in output probability table without entries - result will be inaccurate";
        }
    } else {
        LOG(STATUS) << "All bins have sufficient entries, average number of initial deposits per bin is "
                    << statistics / normalization_table_.size();
    }
}

std::shared_ptr<std::vector<double>> PropagationMap::getNormalizedField() {
    const std::lock_guard field_lock{field_mutex_};

    // Loop over field and apply normalization:
    for(size_t i = 0; i < field_->size(); i++) {
        if(normalization_table_[i / 25] > 0) {
            (*field_)[i] /= static_cast<double>(normalization_table_[i / 25]);
        }
    }
    // Return field
    return field_;
}
