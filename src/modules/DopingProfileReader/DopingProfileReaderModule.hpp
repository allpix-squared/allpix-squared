/**
 * @file
 * @brief Definition of module to read doping concentration maps
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
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
    public:
        /**
         * @brief Constructor for this detector-specific module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        DopingProfileReaderModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);

        /**
         * @brief [Initialise this module]
         */
        void init() override;

    private:
        // General module members
        std::shared_ptr<Detector> detector_;

        /**
         * @brief Read field in the init format and apply it
         * @param field_scale Scaling parameters for the field size in x and y
         */
        FieldData<double> read_field(std::array<double, 2> field_scale);
        static FieldParser<double> field_parser_;

        /**
         * @brief Compare the dimensions of the detector with the field, print warnings
         * @param dimensions Dimensions of the field read from file
         * @param thickness_domain Domain of the thickness where the field is defined
         * @param field_scale The configured scaling parameters of the electric field in x and y
         */
        void check_detector_match(std::array<double, 3> dimensions, std::array<double, 2> field_scale);
    };
} // namespace allpix
