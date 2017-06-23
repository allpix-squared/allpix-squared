/**
 * @file
 * @brief Parameters of a monolithic pixel detector model
 *
 * @copyright MIT License
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

#include "DetectorModel.hpp"

namespace allpix {

    /**
     * @ingroup DetectorModels
     * @brief Model of a monolithic pixel detector. This a model where the sensor is directly connected to the chip.
     *
     * This model is basically already fully implemented in the \ref DetectorModel base class.
     */
    class MonolithicPixelDetectorModel : public DetectorModel {
    public:
        /**
         * @brief Constructs the monolithic pixel detector model
         * @param config Configuration description of the model
         */
        explicit MonolithicPixelDetectorModel(const Configuration& config) : DetectorModel(config) {}
    };
} // namespace allpix

#endif /* ALLPIX_MONOLITHIC_PIXEL_DETECTOR_H */
