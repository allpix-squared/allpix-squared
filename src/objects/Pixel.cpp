/**
 * @file
 * @brief Implementation of pixel object
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "Pixel.hpp"

using namespace allpix;

bool ROOT::Math::operator<(const ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>>& lhs,
                           const ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>>& rhs) {
    if(lhs.x() == rhs.x()) {
        return lhs.y() < rhs.y();
    }
    return lhs.x() < rhs.x();
}

Pixel::Pixel(Pixel::Index index,
             Pixel::Type type,
             ROOT::Math::XYZPoint local_center,
             ROOT::Math::XYZPoint global_center,
             ROOT::Math::XYVector size)
    : index_(std::move(index)), type_(type), local_center_(std::move(local_center)),
      global_center_(std::move(global_center)), size_(std::move(size)) {}

Pixel::Index Pixel::getIndex() const { return index_; }

Pixel::Type Pixel::getType() const { return type_; }

ROOT::Math::XYZPoint Pixel::getLocalCenter() const { return local_center_; }
ROOT::Math::XYZPoint Pixel::getGlobalCenter() const { return global_center_; }
ROOT::Math::XYVector Pixel::getSize() const { return size_; }
