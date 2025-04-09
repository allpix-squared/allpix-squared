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

namespace allpix {
    /**
     * @brief Instance of a propagation map
     *
     * Extends DetectorField to allow easily *setting* values instead of just reading them from an existing map
     */
    class PropagationMap : public DetectorField<FieldTable, 25> {

    public:
        /**
         * @brief Constructs a detector field
         */
        PropagationMap() = default;
    };
} // namespace allpix

#endif /* ALLPIX_PROPAGATION_MAP_H */
