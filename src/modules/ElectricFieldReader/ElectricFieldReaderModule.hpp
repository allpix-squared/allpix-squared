/**
 * @file
 * @brief Definition of module to read electric fields
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
     * @brief Module to read electric fields from INIT format or apply a linear electric field
     *
     * Read the model of the electric field from the config during initialization:
     * - For a linear field create a constant electric field to apply over the whole sensitive device
     * - For the INIT format, reads the specified file and add the electric field grid to the bound detectors
     */
    class ElectricFieldReaderModule : public Module {
        /**
         * @brief Different electric field types
         */
        enum class ElectricField {
            CONSTANT,  ///< Constant electric field
            LINEAR,    ///< Linear electric field
            MESH,      ///< Electric field defined by a mesh
            PARABOLIC, ///< Parabolic electric field
            CUSTOM,    ///< Custom electric field, defined as 3-dimensional function
        };

    public:
        /**
         * @brief Constructor for this detector-specific module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        ElectricFieldReaderModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);

        /**
         * @brief Read electric field and apply it to the bound detectors
         */
        void initialize() override;

    private:
        std::shared_ptr<Detector> detector_;

        /**
         * @brief Create and apply a linear field
         * @param depletion_voltage Depletion voltage of the sensor for the given thickness domain
         * @param thickness_domain Domain of the thickness where the field is defined
         */
        FieldFunction<ROOT::Math::XYZVector> get_linear_field_function(double depletion_voltage,
                                                                       std::pair<double, double> thickness_domain);

        /**
         * @brief Create and apply a parabolic field
         * @param thickness_domain Domain of the thickness where the field is defined
         */
        FieldFunction<ROOT::Math::XYZVector> get_parabolic_field_function(std::pair<double, double> thickness_domain);

        /**
         * @brief Create and apply a custom field from functions
         * @param thickness_domain Domain of the thickness where the field is defined
         * @return Pair with the field function and the deduced field type, CUSTOM or CUSTOM1D
         */
        std::pair<FieldFunction<ROOT::Math::XYZVector>, FieldType> get_custom_field_function();

        /**
         * @brief Read field from a file in init or apf format
         * @return Data of the field read from file
         */
        FieldData<double> read_field();
        static FieldParser<double> field_parser_;

        /**
         * @brief Create output plots of the electric field profile
         */
        void create_output_plots();
    };
} // namespace allpix
