/**
 * @file
 * @brief Definition of module to define magnetic fields
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "tools/field_parser.h"

#include "core/module/Module.hpp"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to define magnetic fields
     *
     * Read the model of the magnetic field from the config during initialization and apply a constant field throughout the
     * whole volume
     */
    class MagneticFieldReaderModule : public Module {
        /**
         * @brief Different magnetic field types
         */
        enum class MagneticField : std::uint8_t {
            CONSTANT, ///< Constant magnetic field
            MESH,     ///< Magnetic field defined by a mesh
        };

    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geoManager Pointer to the global GeometryManager
         */
        MagneticFieldReaderModule(Configuration& config, Messenger* messenger, GeometryManager* geoManager);

        /**
         * @brief Read magnetic field, feed it back to the geometry manager and apply it to the bound detectors
         */
        void initialize() override;

    private:
        GeometryManager* geometryManager_;

        /**
         * @brief Read field from a file in init or apf format
         * @return Data of the field read from file
         */
        FieldData<double> read_field();
        static FieldParser<double> field_parser_;
    };
} // namespace allpix
