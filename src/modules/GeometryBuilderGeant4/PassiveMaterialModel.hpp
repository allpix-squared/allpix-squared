/**
 * @file
 * @brief Base of passive material models
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

/**
 * @defgroup PassiveMaterialModels passive material models
 * @brief Collection of passive material models supported by the framework
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

#include "core/config/Configuration.hpp"
#include "core/config/exceptions.h"
#include "core/utils/log.h"
#include "tools/ROOT.h"

#include "G4VSolid.hh"

namespace allpix {
    /**
     * @ingroup PassiveMaterialModels
     * @brief Base of all passive material models
     *
     * Creates the functions that will be overwritten by the passive material models.
     * The G4VSolid of the passive material and it's optional filling material are defined.
     * The maximum size parameter is defined.
     */
    class PassiveMaterialModel {
    public:
        /**
         * @brief Essential virtual destructor
         */
        virtual ~PassiveMaterialModel() = default;

        /**
         * @brief Virtual function that will return a G4VSolid corresponding to the specific model
         * @return Nullptr if no valid model type is defined
         */
        virtual G4VSolid* getSolid() const { return nullptr; }
        /**
         * @brief Virtual function that will return the maximum size parameter of the model
         * @return 0 if no valid model type is defined
         */
        virtual double getMaxSize() const { return 0; }

    protected:
        /**
         * @brief Constructs the base passive material model
         * @param Configuration with description of the model
         */
        PassiveMaterialModel(Configuration&){};
    };
} // namespace allpix

#endif // ALLPIX_MODULE_PASSIVE_MATERIAL_MODEL_H
