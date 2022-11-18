/**
 * @file
 * @brief Implementation of detector fields
 *
 * @copyright Copyright (c) 2018-2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "DetectorField.hpp"

namespace allpix {

    /*
     * Vector field template specialization of helper function for field flipping
     */
    template <> void flip_vector_components<ROOT::Math::XYZVector>(ROOT::Math::XYZVector& vec, bool x, bool y) {
        vec.SetXYZ((x ? -vec.x() : vec.x()), (y ? -vec.y() : vec.y()), vec.z());
    }

    /*
     * Scalar field template specialization of helper function for field flipping
     * Here, no inversion of the field components is required
     */
    template <> void flip_vector_components<double>(double&, bool, bool) {}
} // namespace allpix
