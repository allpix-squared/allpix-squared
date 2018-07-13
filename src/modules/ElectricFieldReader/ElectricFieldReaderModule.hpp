/**
 * @file
 * @brief Definition of module to read electric fields
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
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
         * Electric field data with three components
         * * The actual field data as shared pointer to vector
         * * An array specifying the number of bins in each dimension
         * * An array containing the physical extent of the field as specified in the file
         */
        using FieldData = std::tuple<std::shared_ptr<std::vector<double>>, std::array<size_t, 3>, std::array<double, 3>>;

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
        void init() override;

    private:
        std::shared_ptr<Detector> detector_;

        /**
         * @brief Create and apply a linear field
         * @param thickness_domain Domain of the thickness where the field is defined
         */
        ElectricFieldFunction get_linear_field_function(double depletion_voltage,
                                                        std::pair<double, double> thickness_domain);

        /**
         * @brief Read field in the init format and apply it
         * @param thickness_domain Domain of the thickness where the field is defined
         */
        FieldData read_init_field(std::pair<double, double> thickness_domain);

        /**
         * @brief Create output plots of the electric field profile
         */
        void create_output_plots();

        /**
         * @brief Compare the dimensions of the detector with the field, print warnings
         * @param dimensions Dimensions of the field read from file
         * @param thickness_domain Domain of the thickness where the field is defined
         */
        void check_detector_match(std::array<double, 3> dimensions, std::pair<double, double> thickness_domain);

        /**
         * @brief Get the electric field from a file name, caching the result between instantiations
         */
        static FieldData get_by_file_name(const std::string& name);
        static std::map<std::string, FieldData> field_map_;
    };
} // namespace allpix
