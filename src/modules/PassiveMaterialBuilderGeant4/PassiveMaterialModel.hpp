/**
 * @file
 * @brief Base of detector models
 *
 * @copyright Copyright (c) 2017-2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

/**
 * @defgroup DetectorModels Detector models
 * @brief Collection of detector models supported by the framework
 */

#ifndef ALLPIX_MODULE_PASSIVE_MATERIAL_MODEL_H
#define ALLPIX_MODULE_PASSIVE_MATERIAL_MODEL_H

#include <array>
#include <string>
#include <utility>

#include <Math/Point2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

#include "core/config/ConfigReader.hpp"
#include "core/config/exceptions.h"
#include "core/utils/log.h"
#include "tools/ROOT.h"

#include <G4PVPlacement.hh>
#include "G4LogicalVolume.hh"
#include "G4VSolid.hh"

namespace allpix {
    /**
     * @ingroup DetectorModels
     * @brief Base of all detector models
     *
     * Implements the minimum required for a detector model. A model always has a pixel grid with a specific pixel size. The
     * pixel grid defines the base size of the sensor, chip and support. Excess length can be specified. Every part of the
     * detector model has a defined center and size which can be overloaded by specialized detector models. The basic
     * detector model also defines the rotation center in the local coordinate system.
     */
    class PassiveMaterialModel {
    public:
        /**
         * @brief Constructs the base detector model
         * @param type Name of the model type
         * @param reader Configuration reader with description of the model
         */
        PassiveMaterialModel(Configuration&);

        /**
         * @brief Essential virtual destructor
         */
        virtual ~PassiveMaterialModel() = default;

        virtual G4VSolid* GetSolid();
        virtual G4VSolid* GetFillingSolid();
        virtual double GetMaxSize();

    private:
        G4VSolid* solid_;
        G4VSolid* filling_solid_;
        double max_size_;
    };
} // namespace allpix

#endif // ALLPIX_MODULE_PASSIVE_MATERIAL_MODEL_H
