/**
 * @file
 * @brief Pixel detector model
 *
 * @copyright Copyright (c) 2017-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_PIXEL_DETECTOR_MODEL_H
#define ALLPIX_PIXEL_DETECTOR_MODEL_H

#include <array>
#include <string>
#include <utility>

#include <Math/Point2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

#include "core/config/ConfigReader.hpp"
#include "core/config/exceptions.h"
#include "core/geometry/DetectorModel.hpp"
#include "core/utils/log.h"
#include "objects/Pixel.hpp"
#include "tools/ROOT.h"

namespace allpix {
    /**
     * @ingroup DetectorModels
     * @brief Model of a generic pixel detector. This model is further extended by specialized pixel detector models.
     */
    class PixelDetectorModel : public DetectorModel {
    public:
        /**
         * @brief Constructs the pixel detector model
         * @param type Name of the model type
         * @param assembly Detector assembly object with information about ASIC and packaging
         * @param reader Configuration reader with description of the model
         * @param header_config Configuration reference holding the unnamed section fo detector configuration
         */
        explicit PixelDetectorModel(std::string type,
                                    const std::shared_ptr<DetectorAssembly>& assembly,
                                    const ConfigReader& reader,
                                    Configuration& header_config);

        /**
         * @brief Returns if a local position is within the sensitive device
         * @param local_pos Position in local coordinates of the detector model
         * @return True if a local position is within the sensor, false otherwise
         */
        bool isWithinSensor(const ROOT::Math::XYZPoint& local_pos) const override;

        /**
         * @brief Calculate exit point of step outside sensor volume from one point inside the sensor (before step) and one
         * point outside (after step).
         * @throws std::invalid_argument if no intersection of track segment with sensor volume can be found
         * @param  inside Position before the step, inside the sensor volume
         * @param  outside  Position after the step, outside the sensor volume
         * @return Exit point of the sensor in local coordinates
         *
         * @note This method uses the Liang-Barsky clipping of a line segment with a box
         */
        ROOT::Math::XYZPoint getSensorIntercept(const ROOT::Math::XYZPoint& inside,
                                                const ROOT::Math::XYZPoint& outside) const override;

        /**
         * @brief Returns if a pixel index is within the grid of pixels defined for the device
         * @param pixel_index Pixel index to be checked
         * @return True if pixel_index is within the pixel grid, false otherwise
         */
        bool isWithinMatrix(const Pixel::Index& pixel_index) const override;

        /**
         * @brief Returns if a set of pixel coordinates is within the grid of pixels defined for the device
         * @param x X- (or column-) coordinate to be checked
         * @param y Y- (or row-) coordinate to be checked
         * @return True if pixel coordinates are within the pixel grid, false otherwise
         */
        bool isWithinMatrix(const int x, const int y) const override;

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

    protected:
        void validate() override;
    };
} // namespace allpix

#endif // ALLPIX_DETECTOR_MODEL_H
