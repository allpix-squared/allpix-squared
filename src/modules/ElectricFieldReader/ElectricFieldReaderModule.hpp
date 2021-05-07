/**
 * @file
 * @brief Definition of module to read electric fields
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
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
            CONSTANT, ///< Constant electric field
            LINEAR,   ///< Linear electric field
            MESH,     ///< Electric field defined by a mesh
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
         * @param thickness_domain Domain of the thickness where the field is defined
         */
        FieldFunction<ROOT::Math::XYZVector> get_linear_field_function(double depletion_voltage,
                                                                       std::pair<double, double> thickness_domain);

        /**
         * @brief Read field from a file in init or apf format and apply it
         * @param thickness_domain Domain of the thickness where the field is defined
         * @param field_scale Scaling parameters for the field size in x and y
         */
        FieldData<double> read_field(std::pair<double, double> thickness_domain, std::array<double, 2> field_scale);
        static FieldParser<double> field_parser_;

        /**
         * @brief Create output plots of the electric field profile
         */
        void create_output_plots();

        /**
         * @brief Compare the dimensions of the detector with the field, print warnings
         * @param dimensions Dimensions of the field read from file
         * @param thickness_domain Domain of the thickness where the field is defined
         * @param field_scale The configured scaling parameters of the electric field in x and y
         */
        void check_detector_match(std::array<double, 3> dimensions,
                                  std::pair<double, double> thickness_domain,
                                  std::array<double, 2> field_scale);
    };
} // namespace allpix
