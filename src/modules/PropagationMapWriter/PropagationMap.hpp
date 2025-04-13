/**
 * @file
 * @brief Definition of detector fields
 *
 * @copyright Copyright (c) 2018-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_PROPAGATION_MAP_H
#define ALLPIX_PROPAGATION_MAP_H

#include "core/geometry/DetectorField.hpp"
#include "core/geometry/DetectorModel.hpp"

#include <mutex>

namespace allpix {
    /**
     * @brief Instance of a propagation map
     *
     * Extends DetectorField to allow easily *setting* values instead of just reading them from an existing map
     */
    class PropagationMap : public DetectorField<FieldTable, 25> {

    public:
        /**
         * @brief Constructs a propagation map
         */
        PropagationMap(const std::shared_ptr<DetectorModel>& model,
                       std::array<size_t, 3> bins,
                       std::array<double, 3> size,
                       FieldMapping mapping,
                       std::array<double, 2> scales,
                       std::array<double, 2> offset,
                       std::pair<double, double> thickness_domain);

        /**
         * @brief Set the propagation map value in the sensor at a position provided in local coordinates
         * @param local_pos Position in the local frame
         * @param table Tabulated probabilities, normalized to total charge
         */
        void add(const ROOT::Math::XYZPoint& local_pos, const FieldTable& table);

        /**
         * @brief Perform some validity checks on the field
         */
        void checkField();

        std::shared_ptr<std::vector<double>> getNormalizedField();

    private:
        std::vector<size_t> normalization_table;
        std::mutex field_mutex_;
    };
} // namespace allpix

#endif /* ALLPIX_PROPAGATION_MAP_H */
