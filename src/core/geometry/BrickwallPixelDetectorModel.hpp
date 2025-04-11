/**
 * @file
 * @brief Pixel detector model
 *
 * @copyright Copyright (c) 2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_BRICKWALL_PIXEL_DETECTOR_MODEL_H
#define ALLPIX_BRICKWALL_PIXEL_DETECTOR_MODEL_H

#include <array>
#include <string>
#include <utility>

#include <Math/Point2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

#include "PixelDetectorModel.hpp"

namespace allpix {
    /**
     * @ingroup DetectorModels
     * @brief Model of a pixel detector with a brick wall layout.
     */
    class BrickwallPixelDetectorModel : public PixelDetectorModel {
    public:
        /**
         * @brief Constructs the pixel detector model
         * @param type Name of the model type
         * @param assembly Detector assembly object with information about ASIC and packaging
         * @param reader Configuration reader with description of the model
         * @param config Configuration reference holding the unnamed section of detector configuration
         */
        explicit BrickwallPixelDetectorModel(std::string type,
                                             const std::shared_ptr<DetectorAssembly>& assembly,
                                             const ConfigReader& reader,
                                             const Configuration& config);

        /**
         * @brief Get local coordinate of the position and rotation center in global frame
         * @note It can be a bit counter intuitive that this is not usually the origin, neither the geometric center of the
         * model, but the geometric center of the sensitive part. This way, the position of the sensing element is invariant
         * under rotations
         *
         * The center coordinate corresponds to the \ref Detector::getPosition "position" in the global frame.
         */
        ROOT::Math::XYZPoint getMatrixCenter() const override;

        /**
         * @brief Get total size of the pixel grid
         * @return Size of the pixel grid
         *
         * @warning The grid has zero thickness
         * @note This is basically a 2D method, but provided in 3D because it is primarily used there
         */
        ROOT::Math::XYZVector getMatrixSize() const override;

        /**
         * @brief Returns if a position is within the grid of pixels defined for the device
         * @param position Position in local coordinates of the detector model
         * @return True if position within the pixel grid, false otherwise
         */
        bool isWithinMatrix(const ROOT::Math::XYZPoint& position) const override;

        /**
         * @brief Returns a pixel center in local coordinates
         * @param x X- (or column-) coordinate of the pixel
         * @param y Y- (or row-) coordinate of the pixel
         * @return Coordinates of the pixel center
         */
        ROOT::Math::XYZPoint getPixelCenter(const int x, const int y) const override;

        /**
         * @brief Return X,Y indices of a pixel corresponding to a local position in a sensor.
         * @param local_pos Position in local coordinates of the detector model
         * @return X,Y pixel indices
         *
         * @note No checks are performed on whether these indices represent an existing pixel or are within the pixel matrix.
         */
        std::pair<int, int> getPixelIndex(const ROOT::Math::XYZPoint& local_pos) const override;

        /**
         * @brief Return a set containing all pixels neighboring the given one with a configurable maximum distance
         * @param idx       Index of the pixel in question
         * @param distance  Distance for pixels to be considered neighbors
         * @return Set of neighboring pixel indices, including the initial pixel
         *
         * @note The returned set should always also include the initial pixel indices the neighbors are calculated for
         */
        std::set<Pixel::Index> getNeighbors(const Pixel::Index& idx, const size_t distance) const override;

        /**
         * @brief Check if two pixel indices are neighbors to each other
         * @param  seed    Initial pixel index
         * @param  entrant Entrant pixel index to be tested
         * @param distance  Distance for pixels to be considered neighbors
         * @return         Boolean whether pixels are neighbors or not
         */
        bool areNeighbors(const Pixel::Index& seed, const Pixel::Index& entrant, const size_t distance) const override;

    private:
        double offset_;
    };
} // namespace allpix

#endif // ALLPIX_BRICKWALL_PIXEL_DETECTOR_MODEL_H
