/**
 * @file
 * @brief Definition of module to read weighting potentials
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
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
     * @brief Module to read weighting potentials
     */
    class WeightingPotentialReaderModule : public Module {
        /**
         * @brief Different weighting potential types
         */
        enum class WeightingPotential {
            PAD,  ///< Weighting potential calculated from geometry of the pad
            MESH, ///< Weighting potential defined by a mesh
        };

    public:
        /**
         * @brief Constructor for this detector-specific module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        WeightingPotentialReaderModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);

        /**
         * @brief Read weighting potential and apply it to the bound detectors
         */
        void initialize() override;

    private:
        std::shared_ptr<Detector> detector_;

        /**
         * @brief Create and apply a weighting potential equivalent to a pixel/pad in a plane condenser
         * @param implant Vector with size of the readout implant in x and y
         * @param thickness_domain Domain of the thickness where the field is defined
         */
        FieldFunction<double> get_pad_potential_function(const ROOT::Math::XYVector& implant,
                                                         std::pair<double, double> thickness_domain);

        /**
         * @brief Read field from a file in init or apf format
         * @return Data of the field read from file
         */
        FieldData<double> read_field();
        static FieldParser<double> field_parser_;

        /**
         * @brief Create output plots of the weighting potential profile
         */
        void create_output_plots();
    };
} // namespace allpix
