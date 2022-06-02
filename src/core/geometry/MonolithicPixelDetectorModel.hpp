/**
 * @file
 * @brief Parameters of a monolithic pixel detector model
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MONOLITHIC_PIXEL_DETECTOR_H
#define ALLPIX_MONOLITHIC_PIXEL_DETECTOR_H

#include <string>
#include <utility>

#include <Math/Cartesian2D.h>
#include <Math/DisplacementVector2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

#include "PixelDetectorModel.hpp"

namespace allpix {

    /**
     * @ingroup DetectorModels
     * @brief Model of a monolithic pixel detector. This a model where sensor and readout electronics are placed within the
     * same silicon wafer.
     *
     * This model is already fully implemented in the \ref PixelDetectorModel base class.
     */
    class MonolithicPixelDetectorModel : public PixelDetectorModel {
    public:
        /**
         * @brief Constructs the monolithic pixel detector model
         * @param type Name of the model type
         * @param reader Configuration reader with description of the model
         */
        explicit MonolithicPixelDetectorModel(std::string type, std::shared_ptr<Chip> chip, const ConfigReader& reader)
            : PixelDetectorModel(std::move(type), chip, reader) {}
    };
} // namespace allpix

#endif /* ALLPIX_MONOLITHIC_PIXEL_DETECTOR_H */
