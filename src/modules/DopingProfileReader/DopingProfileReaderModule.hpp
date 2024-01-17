/**
 * @file
 * @brief Definition of module to read doping concentration maps
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "core/config/Configuration.hpp"
#include "core/geometry/DetectorModel.hpp"
#include "core/messenger/Messenger.hpp"
#include "tools/field_parser.h"

#include "core/module/Module.hpp"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to do function
     *
     * More detailed explanation of module
     */
    class DopingProfileReaderModule : public Module {
        /**
         * @brief Different doping profile types
         */
        enum class DopingProfile {
            CONSTANT, ///< Constant doping concentration
            REGIONS,  ///< Different regions with different doping concentrations
            MESH,     ///< Doping profile defined by a mesh
        };

    public:
        /**
         * @brief Constructor for this detector-specific module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        DopingProfileReaderModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);

        /**
         * @brief Read field maps from file and add to the detector
         */
        void initialize() override;

    private:
        // General module members
        std::shared_ptr<Detector> detector_;

        /**
         * @brief Read field from a file in init or apf format
         * @return Data of the field read from file
         */
        FieldData<double> read_field();
        static FieldParser<double> field_parser_;

        /**
         * @brief Create output plots of the doping profile
         */
        void create_output_plots();
    };
} // namespace allpix
