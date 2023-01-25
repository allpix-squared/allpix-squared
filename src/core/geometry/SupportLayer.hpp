/**
 * @file
 * @brief Definition of support layer
 *
 * @copyright Copyright (c) 2022-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_SUPPORT_LAYER_H
#define ALLPIX_SUPPORT_LAYER_H

#include <Math/Point2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

/**
 * @brief Helper class to hold support layers for a detector model
 */
namespace allpix {
    class SupportLayer {
        friend class DetectorModel;

    public:
        /**
         * @brief Get the center of the support layer
         * @return Center of the support layer
         */
        const ROOT::Math::XYZPoint& getCenter() const { return center_; }
        /**
         * @brief Get the size of the support layer
         * @return Size of the support layer
         */
        const ROOT::Math::XYZVector& getSize() const { return size_; }
        /**
         * @brief Get the material of the support layer
         * @return Support material
         */
        const std::string& getMaterial() const { return material_; }
        /**
         * @brief Return if the support layer contains a hole
         * @return True if the support layer has a hole, false otherwise
         */
        bool hasHole() const { return hole_size_.x() > 1e-9 && hole_size_.y() > 1e-9; }

        /**
         * @brief Return the support layer hole type
         * @return support layer hole type
         */
        const std::string& getHoleType() const { return type_; }

        /**
         * @brief Get the center of the hole in the support layer
         * @return Center of the hole
         */
        ROOT::Math::XYZPoint getHoleCenter() const {
            return center_ + ROOT::Math::XYZVector(hole_offset_.x(), hole_offset_.y(), 0);
        }
        /**
         * @brief Get the full size of the hole in the support layer
         * @return Size of the hole
         */
        const ROOT::Math::XYZVector& getHoleSize() const { return hole_size_; }
        /**
         * @brief Get the location of the support layer
         */
        const std::string& getLocation() const { return location_; }

    private:
        /**
         * @brief Constructs a support layer, used in \ref DetectorModel::addSupportLayer
         * @param size Size of the support layer
         * @param offset Offset of the support layer from the center
         * @param material Material of the support layer
         * @param type Type of the hole
         * @param location Location of the support material
         * @param hole_size Size of an optional hole (zero vector if no hole)
         * @param hole_offset Offset of the optional hole from the center of the support layer
         */
        SupportLayer(ROOT::Math::XYZVector size,
                     ROOT::Math::XYZVector offset,
                     std::string material,
                     std::string type,
                     std::string location,
                     ROOT::Math::XYZVector hole_size,
                     ROOT::Math::XYVector hole_offset)
            : size_(std::move(size)), material_(std::move(material)), type_(std::move(type)),
              hole_size_(std::move(hole_size)), offset_(std::move(offset)), hole_offset_(std::move(hole_offset)),
              location_(std::move(location)) {}

        // Actual parameters returned
        ROOT::Math::XYZPoint center_;
        ROOT::Math::XYZVector size_;
        std::string material_;
        std::string type_;
        ROOT::Math::XYZVector hole_size_;

        // Internal parameters to calculate return parameters
        ROOT::Math::XYZVector offset_;
        ROOT::Math::XYVector hole_offset_;
        std::string location_;
    };
} // namespace allpix

#endif // ALLPIX_SUPPORT_LAYER_H
