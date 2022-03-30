/**
 * @file
 * @brief Utility to perform Liang-Barsky clipping checks on volumes
 *
 * @copyright Copyright (c) 2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_LIANG_BARSKY_H
#define ALLPIX_LIANG_BARSKY_H

#include <Math/Point3D.h>
#include <Math/Vector3D.h>

namespace allpix {

    /**
     * @brief Liang–Barsky clipping of a line against faces of a box.
     *
     * See Liang, Y. D., and Barsky, B., "A New Concept and Method for Line Clipping", ACM Transactions on Graphics,
     * 3(1):1–22 for an in-depth explanation. This method requires the position to be in the coordinate system of the box
     * to be tested for intersections, with the box center at its origin and the box sides aligned with the coordinate
     * axes.
     *
     * @param direction Direction vector of the motion
     * @param position Original ("before") position to be considered
     * @param box Size of the box to calculate the intersections with
     * @return Closest intersection with box in the direction indicated by input vector
     *
     * @throws std::invalid_argument if no intersection of track segment with the box volume can be found in positive
     * direction from the given position.
     */
    inline ROOT::Math::XYZPoint LiangBarsky(const ROOT::Math::XYZVector& direction,
                                            const ROOT::Math::XYZPoint& position,
                                            const ROOT::Math::XYZVector& box) {

        auto clip = [](double denominator, double numerator, double& t0, double& t1) {
            if(denominator > 0) {
                if(numerator > denominator * t1) {
                    return false;
                }
                if(numerator > denominator * t0) {
                    t0 = numerator / denominator;
                }
                return true;
            } else if(denominator < 0) {
                if(numerator > denominator * t0) {
                    return false;
                }
                if(numerator > denominator * t1) {
                    t1 = numerator / denominator;
                }
                return true;
            } else {
                return numerator <= 0;
            }
        };

        // Clip the particle track against the six possible box faces
        double t0 = std::numeric_limits<double>::lowest(), t1 = std::numeric_limits<double>::max();
        bool intersect = clip(direction.X(), -position.X() - box.X() / 2, t0, t1) &&
                         clip(-direction.X(), position.X() - box.X() / 2, t0, t1) &&
                         clip(direction.Y(), -position.Y() - box.Y() / 2, t0, t1) &&
                         clip(-direction.Y(), position.Y() - box.Y() / 2, t0, t1) &&
                         clip(direction.Z(), -position.Z() - box.Z() / 2, t0, t1) &&
                         clip(-direction.Z(), position.Z() - box.Z() / 2, t0, t1);

        // The intersection is a point P + t * D. Return closest impact point if positive (i.e. in direction of the motion)
        if(intersect) {
            if(t0 > 0 && t1 > 0) {
                return (position + std::min(t0, t1) * direction);
            } else if(t0 > 0) {
                return (position + t0 * direction);
            } else if(t1 > 0) {
                return (position + t1 * direction);
            }
        }

        // Otherwise: The line does not intersect the box.
        throw std::invalid_argument("no intersection with volume boundaries found");
    }
} // namespace allpix

#endif /* ALLPIX_LIANG_BARSKY_H */
